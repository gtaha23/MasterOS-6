#include "fat.h"
#include "ata.h"

// ── BPB (BIOS Parameter Block) ───
// Sits in the first sector of the disk (the boot sector)

typedef struct __attribute__((packed)) {
    uint8_t  jump[3];
    uint8_t  oem[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  fat_count;
    uint16_t root_entry_count;
    uint16_t total_sectors_16;
    uint8_t  media_type;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t head_count;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
} BPB;

// ── Raw 32-byte directory entry ───

typedef struct __attribute__((packed)) {
    uint8_t  name[8];
    uint8_t  ext[3];
    uint8_t  attributes;
    uint8_t  reserved[10];
    uint16_t time;
    uint16_t date;
    uint16_t first_cluster;
    uint32_t file_size;
} RawDirEntry;

// ── State (filled by fat_init) ───

static BPB    bpb;
static int    initialized = 0;

static uint32_t fat_start;       // LBA of first FAT
static uint32_t root_start;      // LBA of root directory
static uint32_t data_start;      // LBA of first data cluster
static uint32_t root_sectors;    // sectors occupied by root dir

// ── Sector buffer (one sector at a time to keep stack usage low) ───

static uint8_t sector_buf[512];

// ── String helpers ──

static int k_toupper(int c) {
    return (c >= 'a' && c <= 'z') ? c - 32 : c;
}

static int k_strcmp_nocase(const char* a, const char* b) {
    while (*a && *b) {
        int ca = k_toupper((unsigned char)*a);
        int cb = k_toupper((unsigned char)*b);
        if (ca != cb) return ca - cb;
        a++; b++;
    }
    return k_toupper((unsigned char)*a) - k_toupper((unsigned char)*b);
}

// Convert raw 8.3 dir entry name to "FILENAME.EXT" string
static void make_name(const uint8_t* raw_name, const uint8_t* raw_ext, char* out) {
    int i = 0, j = 0;

    // Name part (up to 8 chars, strip trailing spaces)
    while (i < 8 && raw_name[i] != ' ') {
        out[j++] = raw_name[i++];
    }

    // Extension (up to 3 chars, strip trailing spaces)
    int has_ext = 0;
    for (int k = 0; k < 3; k++) {
        if (raw_ext[k] != ' ') { has_ext = 1; break; }
    }

    if (has_ext) {
        out[j++] = '.';
        int k = 0;
        while (k < 3 && raw_ext[k] != ' ') {
            out[j++] = raw_ext[k++];
        }
    }

    out[j] = '\0';
}

// Convert "HELLO.TXT" input to uppercase 8+3 parts for comparison
static void split_83(const char* name, char out_name[8], char out_ext[3]) {
    int i = 0, j = 0;
    for (i = 0; i < 8; i++) out_name[i] = ' ';
    for (i = 0; i < 3; i++) out_ext[i]  = ' ';

    i = 0;
    while (name[i] && name[i] != '.' && i < 8) {
        out_name[i] = k_toupper((unsigned char)name[i]);
        i++;
    }
    if (name[i] == '.') {
        i++;
        j = 0;
        while (name[i] && j < 3) {
            out_ext[j++] = k_toupper((unsigned char)name[i++]);
        }
    }
}

// ── FAT cluster chain ──

// FAT needs 1.5 bytes per entry — read it properly
// fat_buf must be the entire FAT (sectors_per_fat * 512 bytes)
static uint16_t fat_next_cluster(const uint8_t* fat_buf, uint16_t cluster) {
    uint32_t offset = cluster + (cluster / 2); // 1.5 bytes per entry
    uint16_t val;

    val = fat_buf[offset] | (fat_buf[offset + 1] << 8);

    if (cluster & 1)
        val >>= 4;
    else
        val &= 0x0FFF;

    return val;
}

// ── Public API ──

int fat_init() {
    // Read boot sector (LBA 0)
    if (ata_read(0, 1, sector_buf) != 0) return -1;

    // Copy BPB from boot sector
    BPB* b = (BPB*)sector_buf;
    bpb = *b;

    // Compute layout
    fat_start  = bpb.reserved_sectors;
    root_start = fat_start + (bpb.fat_count * bpb.sectors_per_fat);
    root_sectors = ((bpb.root_entry_count * 32) + (bpb.bytes_per_sector - 1))
                   / bpb.bytes_per_sector;
    data_start = root_start + root_sectors;

    initialized = 1;
    return 0;
}

// Total disk capacity in KB, from the BPB
uint32_t fat_get_total_kb() {
    if (!initialized) return 0;

    uint32_t total_sectors = bpb.total_sectors_16
                              ? bpb.total_sectors_16
                              : bpb.total_sectors_32;

    return (total_sectors * bpb.bytes_per_sector) / 1024;
}

// Used space in KB — sums up file sizes in the root directory
// (Note: only accounts for root dir files, not subdirectories)
uint32_t fat_get_used_kb() {
    if (!initialized) return 0;

    uint32_t used_bytes = 0;

    for (uint32_t s = 0; s < root_sectors; s++) {
        if (ata_read(root_start + s, 1, sector_buf) != 0) return 0;

        RawDirEntry* dir = (RawDirEntry*)sector_buf;
        int entries_per_sector = 512 / 32;

        for (int i = 0; i < entries_per_sector; i++) {
            uint8_t first = dir[i].name[0];
            if (first == 0x00) goto done;
            if (first == 0xE5) continue;
            if (dir[i].attributes & 0x08) continue; // volume label
            if (dir[i].attributes & 0x10) continue; // directory

            used_bytes += dir[i].file_size;
        }
    }

done:
    return used_bytes;
}

int fat_list(Fat12Entry* entries, int max) {
    if (!initialized) return -1;

    int count = 0;

    for (uint32_t s = 0; s < root_sectors && count < max; s++) {
        if (ata_read(root_start + s, 1, sector_buf) != 0) return -1;

        RawDirEntry* dir = (RawDirEntry*)sector_buf;
        int entries_per_sector = 512 / 32;

        for (int i = 0; i < entries_per_sector && count < max; i++) {
            uint8_t first = dir[i].name[0];

            if (first == 0x00) goto done;       // no more entries
            if (first == 0xE5) continue;        // deleted
            if (dir[i].attributes & 0x08) continue; // volume label
            if (dir[i].attributes & 0x10) continue; // directory (skip for now)

            make_name(dir[i].name, dir[i].ext, entries[count].name);
            entries[count].size          = dir[i].file_size;
            entries[count].first_cluster = dir[i].first_cluster;
            count++;
        }
    }

done:
    return count;
}

int fat_read(const char* name, uint8_t* buf, uint32_t buf_size) {
    if (!initialized) return -1;

    // ── Step 1: find the file in root directory ──
    RawDirEntry found;
    int file_found = 0;

    // Use a second local sector buffer so we don't clobber sector_buf
    static uint8_t dir_buf[512];

    for (uint32_t s = 0; s < root_sectors && !file_found; s++) {
        if (ata_read(root_start + s, 1, dir_buf) != 0) return -1;

        RawDirEntry* dir = (RawDirEntry*)dir_buf;
        for (int i = 0; i < 512 / 32; i++) {
            uint8_t first = dir[i].name[0];
            if (first == 0x00) goto search_done;  // end of directory
            if (first == 0xE5) continue;           // deleted entry
            if (dir[i].attributes & 0x08) continue; // volume label
            if (dir[i].attributes & 0x10) continue; // subdirectory

            // Build the display name (same as dir does) and compare
            char entry_name[FAT_NAME_LEN];
            make_name(dir[i].name, dir[i].ext, entry_name);

            if (k_strcmp_nocase(entry_name, name) == 0) {
                found = dir[i];
                file_found = 1;
                break;
            }
        }
    }

search_done:
    if (!file_found) return -1;

    // ── Step 2: load FAT one sector at a time ───
    // Max FAT FAT size = 9 sectors on a 1.44MB floppy
    static uint8_t fat_buf[18 * 512]; // 18 sectors = safe upper bound
    uint32_t fat_secs = bpb.sectors_per_fat;
    if (fat_secs > 18) fat_secs = 18;

    for (uint32_t s = 0; s < fat_secs; s++) {
        if (ata_read(fat_start + s, 1, fat_buf + s * 512) != 0) return -1;
    }

    // ── Step 3: walk cluster chain and copy data ─
    uint32_t bytes_read = 0;
    uint16_t cluster   = found.first_cluster;
    uint32_t file_size = found.file_size;

    // Sanity check
    if (cluster < 2 || file_size == 0) return 0;

    while (cluster >= 0x002 && cluster <= 0xFEF) {
        uint32_t lba = data_start + (uint32_t)(cluster - 2) * bpb.sectors_per_cluster;

        for (uint8_t s = 0; s < bpb.sectors_per_cluster; s++) {
            if (ata_read(lba + s, 1, sector_buf) != 0) return -1;

            uint32_t to_copy = 512;
            if (bytes_read + to_copy > file_size)
                to_copy = file_size - bytes_read;
            if (bytes_read + to_copy > buf_size)
                to_copy = buf_size - bytes_read;

            uint8_t* src = sector_buf;
            uint8_t* dst = buf + bytes_read;
            for (uint32_t k = 0; k < to_copy; k++) dst[k] = src[k];

            bytes_read += to_copy;
            if (bytes_read >= file_size || bytes_read >= buf_size) goto read_done;
        }

        cluster = fat_next_cluster(fat_buf, cluster);

        // Guard against infinite loops from corrupt FAT
        if (cluster < 2) break;
    }

read_done:
    return (int)bytes_read;
}

// ── Write support ──

// Find a free cluster (FAT entry == 0). Returns cluster number, or 0 if none.
static uint16_t fat_find_free_cluster(uint8_t* fat_buf, uint32_t total_clusters) {
    for (uint32_t c = 2; c < total_clusters + 2; c++) {
        if (fat_next_cluster(fat_buf, c) == 0) return (uint16_t)c;
    }
    return 0;
}

// Set FAT entry for a cluster (1.5 byte packed write)
static void fat_set_cluster(uint8_t* fat_buf, uint16_t cluster, uint16_t value) {
    uint32_t offset = cluster + (cluster / 2);

    if (cluster & 1) {
        // Odd cluster: high 12 bits live in this entry
        fat_buf[offset]   = (fat_buf[offset] & 0x0F) | ((value & 0x0F) << 4);
        fat_buf[offset+1] = (value >> 4) & 0xFF;
    } else {
        // Even cluster: low 12 bits live in this entry
        fat_buf[offset]   = value & 0xFF;
        fat_buf[offset+1] = (fat_buf[offset+1] & 0xF0) | ((value >> 8) & 0x0F);
    }
}

// Delete a file by name. Returns 0 on success, -1 if not found or error.
int fat_delete(const char* name) {
    if (!initialized) return -1;

    static uint8_t dbuf[512];

    for (uint32_t s = 0; s < root_sectors; s++) {
        if (ata_read(root_start + s, 1, dbuf) != 0) return -1;

        RawDirEntry* dir = (RawDirEntry*)dbuf;
        int changed = 0;

        for (int i = 0; i < 512 / 32; i++) {
            uint8_t first = dir[i].name[0];
            if (first == 0x00) goto not_found;
            if (first == 0xE5) continue;
            if (dir[i].attributes & 0x08) continue;
            if (dir[i].attributes & 0x10) continue;

            char entry_name[FAT_NAME_LEN];
            make_name(dir[i].name, dir[i].ext, entry_name);

            if (k_strcmp_nocase(entry_name, name) == 0) {
                // Mark deleted
                dir[i].name[0] = 0xE5;
                changed = 1;

                // Free the cluster chain
                static uint8_t fat_buf[18 * 512];
                uint32_t fat_secs = bpb.sectors_per_fat;
                if (fat_secs > 18) fat_secs = 18;
                for (uint32_t fs = 0; fs < fat_secs; fs++) {
                    if (ata_read(fat_start + fs, 1, fat_buf + fs * 512) != 0) return -1;
                }

                uint16_t cluster = dir[i].first_cluster;
                while (cluster >= 2 && cluster <= 0xFEF) {
                    uint16_t next = fat_next_cluster(fat_buf, cluster);
                    fat_set_cluster(fat_buf, cluster, 0x000);
                    cluster = next;
                }

                // Write FAT back (all copies)
                for (uint8_t f = 0; f < bpb.fat_count; f++) {
                    uint32_t this_fat_start = fat_start + f * bpb.sectors_per_fat;
                    for (uint32_t fs = 0; fs < fat_secs; fs++) {
                        if (ata_write(this_fat_start + fs, 1, fat_buf + fs * 512) != 0) return -1;
                    }
                }

                break;
            }
        }

        if (changed) {
            if (ata_write(root_start + s, 1, dbuf) != 0) return -1;
            return 0;
        }
    }

not_found:
    return -1;
}

// Rename a file. Returns 0 on success, -1 on error/not found.
int fat_rename(const char* old_name, const char* new_name) {
    if (!initialized) return -1;

    char want_name[8], want_ext[3];
    split_83(new_name, want_name, want_ext);

    static uint8_t dbuf[512];

    for (uint32_t s = 0; s < root_sectors; s++) {
        if (ata_read(root_start + s, 1, dbuf) != 0) return -1;

        RawDirEntry* dir = (RawDirEntry*)dbuf;
        int changed = 0;

        for (int i = 0; i < 512 / 32; i++) {
            uint8_t first = dir[i].name[0];
            if (first == 0x00) goto not_found2;
            if (first == 0xE5) continue;
            if (dir[i].attributes & 0x08) continue;
            if (dir[i].attributes & 0x10) continue;

            char entry_name[FAT_NAME_LEN];
            make_name(dir[i].name, dir[i].ext, entry_name);

            if (k_strcmp_nocase(entry_name, old_name) == 0) {
                for (int k = 0; k < 8; k++) dir[i].name[k] = (uint8_t)want_name[k];
                for (int k = 0; k < 3; k++) dir[i].ext[k]  = (uint8_t)want_ext[k];
                changed = 1;
                break;
            }
        }

        if (changed) {
            if (ata_write(root_start + s, 1, dbuf) != 0) return -1;
            return 0;
        }
    }

not_found2:
    return -1;
}

// Write a new file with given name and data. Returns 0 on success, -1 on error.
// Overwrites if a file with the same name already exists (deletes it first).
int fat_write(const char* name, const uint8_t* data, uint32_t size) {
    if (!initialized) return -1;

    // If file already exists, delete it first (simplifies overwrite logic)
    fat_delete(name); // ignore return — ok if it didn't exist

    char want_name[8], want_ext[3];
    split_83(name, want_name, want_ext);

    // ── Load FAT ──
    static uint8_t fat_buf[18 * 512];
    uint32_t fat_secs = bpb.sectors_per_fat;
    if (fat_secs > 18) fat_secs = 18;
    for (uint32_t fs = 0; fs < fat_secs; fs++) {
        if (ata_read(fat_start + fs, 1, fat_buf + fs * 512) != 0) return -1;
    }

    uint32_t total_clusters = (fat_secs * 512 * 2) / 3; // rough, fine for our sizes

    // ── Allocate clusters and write data ──
    uint32_t bytes_per_cluster = bpb.bytes_per_sector * bpb.sectors_per_cluster;
    uint32_t bytes_left = size;
    uint16_t first_cluster = 0;
    uint16_t prev_cluster = 0;

    while (bytes_left > 0) {
        uint16_t cluster = fat_find_free_cluster(fat_buf, total_clusters);
        if (cluster == 0) return -1; // disk full

        if (first_cluster == 0) first_cluster = cluster;
        if (prev_cluster != 0) fat_set_cluster(fat_buf, prev_cluster, cluster);

        // Mark this cluster as used (temporarily EOF, fixed up if chain continues)
        fat_set_cluster(fat_buf, cluster, 0xFFF);

        uint32_t lba = data_start + (uint32_t)(cluster - 2) * bpb.sectors_per_cluster;
        uint32_t offset = size - bytes_left;

        for (uint8_t sct = 0; sct < bpb.sectors_per_cluster; sct++) {
            uint8_t buf[512];
            for (int k = 0; k < 512; k++) buf[k] = 0;

            uint32_t to_copy = 512;
            if (to_copy > bytes_left) to_copy = bytes_left;

            for (uint32_t k = 0; k < to_copy; k++) buf[k] = data[offset + k];

            if (ata_write(lba + sct, 1, buf) != 0) return -1;

            bytes_left -= to_copy;
            offset += to_copy;
            if (bytes_left == 0) break;
        }

        prev_cluster = cluster;
    }

    // ── Write FAT back (all copies) ───
    for (uint8_t f = 0; f < bpb.fat_count; f++) {
        uint32_t this_fat_start = fat_start + f * bpb.sectors_per_fat;
        for (uint32_t fs = 0; fs < fat_secs; fs++) {
            if (ata_write(this_fat_start + fs, 1, fat_buf + fs * 512) != 0) return -1;
        }
    }

    // ── Find a free directory entry and write it ───
    static uint8_t dbuf[512];
    for (uint32_t s = 0; s < root_sectors; s++) {
        if (ata_read(root_start + s, 1, dbuf) != 0) return -1;

        RawDirEntry* dir = (RawDirEntry*)dbuf;
        for (int i = 0; i < 512 / 32; i++) {
            uint8_t first = dir[i].name[0];
            if (first == 0x00 || first == 0xE5) {
                for (int k = 0; k < 8; k++) dir[i].name[k] = (uint8_t)want_name[k];
                for (int k = 0; k < 3; k++) dir[i].ext[k]  = (uint8_t)want_ext[k];
                dir[i].attributes    = 0x00;
                for (int k = 0; k < 10; k++) dir[i].reserved[k] = 0;
                dir[i].time          = 0;
                dir[i].date          = 0;
                dir[i].first_cluster = first_cluster;
                dir[i].file_size     = size;

                return ata_write(root_start + s, 1, dbuf);
            }
        }
    }

    return -1; // root directory full
}
