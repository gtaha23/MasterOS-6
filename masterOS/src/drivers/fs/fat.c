#include "fat.h"
#include "ata.h"
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
static BPB      bpb;
static int      initialized     = 0;
static uint32_t fat_start       = 0;
static uint32_t root_start      = 0;
static uint32_t data_start      = 0;
static uint32_t root_sectors    = 0;
static uint32_t current_dir_cluster = 0;
static char     current_path[256]   = "/";
static uint8_t  sector_buf[512];
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
static void k_strcpy(char* dst, const char* src) {
    while (*src) *dst++ = *src++;
    *dst = '\0';
}
static int k_strlen(const char* s) {
    int i = 0; while (s[i]) i++; return i;
}
static void make_name(const uint8_t* raw_name, const uint8_t* raw_ext, char* out) {
    int i = 0, j = 0;
    while (i < 8 && raw_name[i] != ' ') out[j++] = raw_name[i++];
    int has_ext = 0;
    for (int k = 0; k < 3; k++) if (raw_ext[k] != ' ') { has_ext = 1; break; }
    if (has_ext) {
        out[j++] = '.';
        int k = 0;
        while (k < 3 && raw_ext[k] != ' ') out[j++] = raw_ext[k++];
    }
    out[j] = '\0';
}
static void split_83(const char* name, char out_name[8], char out_ext[3]) {
    int i = 0, j = 0;
    for (i = 0; i < 8; i++) out_name[i] = ' ';
    for (i = 0; i < 3; i++) out_ext[i]  = ' ';
    i = 0;
    while (name[i] && name[i] != '.' && i < 8) {
        out_name[i] = k_toupper((unsigned char)name[i]); i++;
    }
    if (name[i] == '.') {
        i++; j = 0;
        while (name[i] && j < 3) out_ext[j++] = k_toupper((unsigned char)name[i++]);
    }
}
static uint16_t fat_next_cluster(const uint8_t* fat_buf, uint16_t cluster) {
    uint32_t offset = cluster + (cluster / 2);
    uint16_t val = fat_buf[offset] | (fat_buf[offset + 1] << 8);
    if (cluster & 1) val >>= 4; else val &= 0x0FFF;
    return val;
}
static void fat_set_cluster(uint8_t* fat_buf, uint16_t cluster, uint16_t value) {
    uint32_t offset = cluster + (cluster / 2);
    if (cluster & 1) {
        fat_buf[offset]   = (fat_buf[offset] & 0x0F) | ((value & 0x0F) << 4);
        fat_buf[offset+1] = (value >> 4) & 0xFF;
    } else {
        fat_buf[offset]   = value & 0xFF;
        fat_buf[offset+1] = (fat_buf[offset+1] & 0xF0) | ((value >> 8) & 0x0F);
    }
}
static uint16_t fat_find_free_cluster(uint8_t* fat_buf, uint32_t total_clusters) {
    for (uint32_t c = 2; c < total_clusters + 2; c++)
        if (fat_next_cluster(fat_buf, c) == 0) return (uint16_t)c;
    return 0;
}
static int load_fat(uint8_t* fat_buf, uint32_t fat_secs) {
    for (uint32_t s = 0; s < fat_secs; s++)
        if (ata_read(fat_start + s, 1, fat_buf + s * 512) != 0) return -1;
    return 0;
}
static int write_fat(uint8_t* fat_buf, uint32_t fat_secs) {
    for (uint8_t f = 0; f < bpb.fat_count; f++) {
        uint32_t start = fat_start + f * bpb.sectors_per_fat;
        for (uint32_t s = 0; s < fat_secs; s++)
            if (ata_write(start + s, 1, fat_buf + s * 512) != 0) return -1;
    }
    return 0;
}
static int read_dir_sector(uint32_t s, uint8_t* buf,
                            uint32_t dir_cluster,
                            uint8_t* fat_buf, uint32_t fat_secs) {
    if (dir_cluster == 0) {
        if (s >= root_sectors) return 1; 
        return ata_read(root_start + s, 1, buf);
    }
    uint32_t spc   = bpb.sectors_per_cluster;
    uint32_t cidx  = s / spc;   
    uint32_t soff  = s % spc;   
    uint16_t cluster = (uint16_t)dir_cluster;
    for (uint32_t i = 0; i < cidx; i++) {
        cluster = fat_next_cluster(fat_buf, cluster);
        if (cluster < 2 || cluster >= 0xFF8) return 1; 
    }
    uint32_t lba = data_start + (uint32_t)(cluster - 2) * spc + soff;
    return ata_read(lba, 1, buf);
}
int fat_init() {
    if (ata_read(0, 1, sector_buf) != 0) return -1;
    BPB* b = (BPB*)sector_buf;
    bpb = *b;
    fat_start    = bpb.reserved_sectors;
    root_start   = fat_start + (bpb.fat_count * bpb.sectors_per_fat);
    root_sectors = ((bpb.root_entry_count * 32) + (bpb.bytes_per_sector - 1))
                   / bpb.bytes_per_sector;
    data_start   = root_start + root_sectors;
    initialized  = 1;
    return 0;
}
uint32_t fat_get_total_kb() {
    if (!initialized) return 0;
    uint32_t total = bpb.total_sectors_16 ? bpb.total_sectors_16 : bpb.total_sectors_32;
    return (total * bpb.bytes_per_sector) / 1024;
}
uint32_t fat_get_used_kb() {
    if (!initialized) return 0;
    uint32_t used = 0;
    for (uint32_t s = 0; s < root_sectors; s++) {
        if (ata_read(root_start + s, 1, sector_buf) != 0) return 0;
        RawDirEntry* dir = (RawDirEntry*)sector_buf;
        for (int i = 0; i < 512/32; i++) {
            uint8_t f = dir[i].name[0];
            if (f == 0x00) return used;
            if (f == 0xE5 || (dir[i].attributes & 0x08)) continue;
            used += dir[i].file_size;
        }
    }
    return used;
}
uint32_t fat_get_current_cluster() { return current_dir_cluster; }
const char* fat_get_current_path() { return current_path; }
void fat_set_current(uint32_t cluster, const char* path) {
    current_dir_cluster = cluster;
    k_strcpy(current_path, path);
}
int fat_list(Fat12Entry* entries, int max) {
    if (!initialized) return -1;
    static uint8_t fat_buf[18 * 512];
    uint32_t fat_secs = bpb.sectors_per_fat; if (fat_secs > 18) fat_secs = 18;
    if (load_fat(fat_buf, fat_secs) != 0) return -1;
    int count = 0;
    for (uint32_t s = 0; count < max; s++) {
        int r = read_dir_sector(s, sector_buf, current_dir_cluster, fat_buf, fat_secs);
        if (r != 0) break;
        RawDirEntry* dir = (RawDirEntry*)sector_buf;
        for (int i = 0; i < 512/32 && count < max; i++) {
            uint8_t first = dir[i].name[0];
            if (first == 0x00) goto done;
            if (first == 0xE5) continue;
            if (dir[i].attributes & 0x08) continue; 
            if (dir[i].name[0] == '.' ) continue;
            make_name(dir[i].name, dir[i].ext, entries[count].name);
            entries[count].size          = dir[i].file_size;
            entries[count].first_cluster = dir[i].first_cluster;
            entries[count].is_dir        = (dir[i].attributes & 0x10) ? 1 : 0;
            count++;
        }
    }
done:
    return count;
}
int fat_read(const char* name, uint8_t* buf, uint32_t buf_size) {
    if (!initialized) return -1;
    static uint8_t fat_buf[18 * 512];
    uint32_t fat_secs = bpb.sectors_per_fat; if (fat_secs > 18) fat_secs = 18;
    if (load_fat(fat_buf, fat_secs) != 0) return -1;
    RawDirEntry found; int file_found = 0;
    static uint8_t dir_buf[512];
    for (uint32_t s = 0; !file_found; s++) {
        int r = read_dir_sector(s, dir_buf, current_dir_cluster, fat_buf, fat_secs);
        if (r != 0) break;
        RawDirEntry* dir = (RawDirEntry*)dir_buf;
        for (int i = 0; i < 512/32; i++) {
            uint8_t first = dir[i].name[0];
            if (first == 0x00) goto search_done;
            if (first == 0xE5) continue;
            if (dir[i].attributes & 0x08) continue;
            if (dir[i].attributes & 0x10) continue;
            char entry_name[FAT_NAME_LEN];
            make_name(dir[i].name, dir[i].ext, entry_name);
            if (k_strcmp_nocase(entry_name, name) == 0) {
                found = dir[i]; file_found = 1; break;
            }
        }
    }
search_done:
    if (!file_found) return -1;
    uint32_t bytes_read = 0;
    uint16_t cluster    = found.first_cluster;
    uint32_t file_size  = found.file_size;
    if (cluster < 2 || file_size == 0) return 0;
    while (cluster >= 0x002 && cluster <= 0xFEF) {
        uint32_t lba = data_start + (uint32_t)(cluster - 2) * bpb.sectors_per_cluster;
        for (uint8_t s = 0; s < bpb.sectors_per_cluster; s++) {
            if (ata_read(lba + s, 1, sector_buf) != 0) return -1;
            uint32_t to_copy = 512;
            if (bytes_read + to_copy > file_size)  to_copy = file_size  - bytes_read;
            if (bytes_read + to_copy > buf_size)   to_copy = buf_size   - bytes_read;
            for (uint32_t k = 0; k < to_copy; k++) buf[bytes_read + k] = sector_buf[k];
            bytes_read += to_copy;
            if (bytes_read >= file_size || bytes_read >= buf_size) goto read_done;
        }
        cluster = fat_next_cluster(fat_buf, cluster);
        if (cluster < 2) break;
    }
read_done:
    return (int)bytes_read;
}
int fat_delete(const char* name) {
    if (!initialized) return -1;
    static uint8_t fat_buf[18 * 512];
    uint32_t fat_secs = bpb.sectors_per_fat; if (fat_secs > 18) fat_secs = 18;
    if (load_fat(fat_buf, fat_secs) != 0) return -1;
    static uint8_t dbuf[512];
    for (uint32_t s = 0; ; s++) {
        int r = read_dir_sector(s, dbuf, current_dir_cluster, fat_buf, fat_secs);
        if (r != 0) break;
        RawDirEntry* dir = (RawDirEntry*)dbuf;
        int changed = 0;
        for (int i = 0; i < 512/32; i++) {
            uint8_t first = dir[i].name[0];
            if (first == 0x00) goto not_found;
            if (first == 0xE5 || (dir[i].attributes & 0x08) || (dir[i].attributes & 0x10)) continue;
            char entry_name[FAT_NAME_LEN];
            make_name(dir[i].name, dir[i].ext, entry_name);
            if (k_strcmp_nocase(entry_name, name) == 0) {
                uint16_t cluster = dir[i].first_cluster;
                while (cluster >= 2 && cluster <= 0xFEF) {
                    uint16_t next = fat_next_cluster(fat_buf, cluster);
                    fat_set_cluster(fat_buf, cluster, 0x000);
                    cluster = next;
                }
                write_fat(fat_buf, fat_secs);
                dir[i].name[0] = 0xE5;
                changed = 1; break;
            }
        }
        if (changed) {
            if (current_dir_cluster == 0)
                return ata_write(root_start + s, 1, dbuf);
            else {
                uint32_t spc = bpb.sectors_per_cluster;
                uint32_t cidx = s / spc, soff = s % spc;
                uint16_t cluster = (uint16_t)current_dir_cluster;
                for (uint32_t i = 0; i < cidx; i++) cluster = fat_next_cluster(fat_buf, cluster);
                uint32_t lba = data_start + (uint32_t)(cluster - 2) * spc + soff;
                return ata_write(lba, 1, dbuf);
            }
        }
    }
not_found:
    return -1;
}
int fat_rename(const char* old_name, const char* new_name) {
    if (!initialized) return -1;
    char want_name[8], want_ext[3];
    split_83(new_name, want_name, want_ext);
    static uint8_t fat_buf[18 * 512];
    uint32_t fat_secs = bpb.sectors_per_fat; if (fat_secs > 18) fat_secs = 18;
    if (load_fat(fat_buf, fat_secs) != 0) return -1;
    static uint8_t dbuf[512];
    for (uint32_t s = 0; ; s++) {
        int r = read_dir_sector(s, dbuf, current_dir_cluster, fat_buf, fat_secs);
        if (r != 0) break;
        RawDirEntry* dir = (RawDirEntry*)dbuf;
        int changed = 0;
        for (int i = 0; i < 512/32; i++) {
            uint8_t first = dir[i].name[0];
            if (first == 0x00) goto not_found2;
            if (first == 0xE5 || (dir[i].attributes & 0x08) || (dir[i].attributes & 0x10)) continue;
            char entry_name[FAT_NAME_LEN];
            make_name(dir[i].name, dir[i].ext, entry_name);
            if (k_strcmp_nocase(entry_name, old_name) == 0) {
                for (int k = 0; k < 8; k++) dir[i].name[k] = (uint8_t)want_name[k];
                for (int k = 0; k < 3; k++) dir[i].ext[k]  = (uint8_t)want_ext[k];
                changed = 1; break;
            }
        }
        if (changed) {
            if (current_dir_cluster == 0) return ata_write(root_start + s, 1, dbuf);
            else {
                uint32_t spc = bpb.sectors_per_cluster;
                uint16_t cluster = (uint16_t)current_dir_cluster;
                for (uint32_t i = 0; i < s/spc; i++) cluster = fat_next_cluster(fat_buf, cluster);
                return ata_write(data_start + (uint32_t)(cluster-2)*spc + s%spc, 1, dbuf);
            }
        }
    }
not_found2:
    return -1;
}
int fat_write(const char* name, const uint8_t* data, uint32_t size) {
    if (!initialized) return -1;
    fat_delete(name);
    char want_name[8], want_ext[3];
    split_83(name, want_name, want_ext);
    static uint8_t fat_buf[18 * 512];
    uint32_t fat_secs = bpb.sectors_per_fat; if (fat_secs > 18) fat_secs = 18;
    if (load_fat(fat_buf, fat_secs) != 0) return -1;
    uint32_t total_clusters = (fat_secs * 512 * 2) / 3;
    uint32_t bytes_left = size;
    uint16_t first_cluster = 0, prev_cluster = 0;
    while (bytes_left > 0) {
        uint16_t cluster = fat_find_free_cluster(fat_buf, total_clusters);
        if (cluster == 0) return -1;
        if (first_cluster == 0) first_cluster = cluster;
        if (prev_cluster != 0) fat_set_cluster(fat_buf, prev_cluster, cluster);
        fat_set_cluster(fat_buf, cluster, 0xFFF);
        uint32_t lba    = data_start + (uint32_t)(cluster - 2) * bpb.sectors_per_cluster;
        uint32_t offset = size - bytes_left;
        for (uint8_t sct = 0; sct < bpb.sectors_per_cluster; sct++) {
            uint8_t buf[512]; for (int k = 0; k < 512; k++) buf[k] = 0;
            uint32_t to_copy = (bytes_left < 512) ? bytes_left : 512;
            for (uint32_t k = 0; k < to_copy; k++) buf[k] = data[offset + k];
            if (ata_write(lba + sct, 1, buf) != 0) return -1;
            bytes_left -= to_copy; offset += to_copy;
            if (bytes_left == 0) break;
        }
        prev_cluster = cluster;
    }
    if (write_fat(fat_buf, fat_secs) != 0) return -1;
    static uint8_t dbuf[512];
    for (uint32_t s = 0; ; s++) {
        int r = read_dir_sector(s, dbuf, current_dir_cluster, fat_buf, fat_secs);
        if (r != 0) break;
        RawDirEntry* dir = (RawDirEntry*)dbuf;
        for (int i = 0; i < 512/32; i++) {
            uint8_t first = dir[i].name[0];
            if (first == 0x00 || first == 0xE5) {
                for (int k = 0; k < 8; k++) dir[i].name[k] = (uint8_t)want_name[k];
                for (int k = 0; k < 3; k++) dir[i].ext[k]  = (uint8_t)want_ext[k];
                dir[i].attributes = 0x00;
                for (int k = 0; k < 10; k++) dir[i].reserved[k] = 0;
                dir[i].time = dir[i].date = 0;
                dir[i].first_cluster = first_cluster;
                dir[i].file_size     = size;
                if (current_dir_cluster == 0) return ata_write(root_start + s, 1, dbuf);
                else {
                    uint32_t spc = bpb.sectors_per_cluster;
                    uint16_t cluster = (uint16_t)current_dir_cluster;
                    for (uint32_t i2 = 0; i2 < s/spc; i2++) cluster = fat_next_cluster(fat_buf, cluster);
                    return ata_write(data_start + (uint32_t)(cluster-2)*spc + s%spc, 1, dbuf);
                }
            }
        }
    }
    return -1;
}
int fat_mkdir(const char* name) {
    if (!initialized) return -1;
    char want_name[8], want_ext[3];
    split_83(name, want_name, want_ext);
    static uint8_t fat_buf[18 * 512];
    uint32_t fat_secs = bpb.sectors_per_fat; if (fat_secs > 18) fat_secs = 18;
    if (load_fat(fat_buf, fat_secs) != 0) return -1;
    uint32_t total_clusters = (fat_secs * 512 * 2) / 3;
    uint16_t new_cluster = fat_find_free_cluster(fat_buf, total_clusters);
    if (new_cluster == 0) return -1;
    fat_set_cluster(fat_buf, new_cluster, 0xFFF);
    if (write_fat(fat_buf, fat_secs) != 0) return -1;
    uint8_t cluster_buf[512];
    for (int k = 0; k < 512; k++) cluster_buf[k] = 0;
    RawDirEntry* entries = (RawDirEntry*)cluster_buf;
    for (int k = 0; k < 8; k++) entries[0].name[k] = ' ';
    for (int k = 0; k < 3; k++) entries[0].ext[k]  = ' ';
    entries[0].name[0]      = '.';
    entries[0].attributes   = 0x10;
    entries[0].first_cluster = new_cluster;
    entries[0].file_size    = 0;
    for (int k = 0; k < 8; k++) entries[1].name[k] = ' ';
    for (int k = 0; k < 3; k++) entries[1].ext[k]  = ' ';
    entries[1].name[0]      = '.';
    entries[1].name[1]      = '.';
    entries[1].attributes   = 0x10;
    entries[1].first_cluster = (uint16_t)current_dir_cluster;
    entries[1].file_size    = 0;
    uint32_t lba = data_start + (uint32_t)(new_cluster - 2) * bpb.sectors_per_cluster;
    if (ata_write(lba, 1, cluster_buf) != 0) return -1;
    static uint8_t dbuf[512];
    for (uint32_t s = 0; ; s++) {
        int r = read_dir_sector(s, dbuf, current_dir_cluster, fat_buf, fat_secs);
        if (r != 0) break;
        RawDirEntry* dir = (RawDirEntry*)dbuf;
        for (int i = 0; i < 512/32; i++) {
            uint8_t first = dir[i].name[0];
            if (first == 0x00 || first == 0xE5) {
                for (int k = 0; k < 8; k++) dir[i].name[k] = (uint8_t)want_name[k];
                for (int k = 0; k < 3; k++) dir[i].ext[k]  = (uint8_t)want_ext[k];
                dir[i].attributes    = 0x10;
                for (int k = 0; k < 10; k++) dir[i].reserved[k] = 0;
                dir[i].time = dir[i].date = 0;
                dir[i].first_cluster = new_cluster;
                dir[i].file_size     = 0;
                if (current_dir_cluster == 0) return ata_write(root_start + s, 1, dbuf);
                else {
                    uint32_t spc = bpb.sectors_per_cluster;
                    uint16_t cluster = (uint16_t)current_dir_cluster;
                    for (uint32_t i2 = 0; i2 < s/spc; i2++) cluster = fat_next_cluster(fat_buf, cluster);
                    return ata_write(data_start + (uint32_t)(cluster-2)*spc + s%spc, 1, dbuf);
                }
            }
        }
    }
    return -1;
}
int fat_cd(const char* name) {
    if (!initialized) return -1;
    if (name[0] == '.' && name[1] == '.' && name[2] == '\0') {
        current_dir_cluster = 0;
        k_strcpy(current_path, "/");
        return 0;
    }
    static uint8_t fat_buf[18 * 512];
    uint32_t fat_secs = bpb.sectors_per_fat; if (fat_secs > 18) fat_secs = 18;
    if (load_fat(fat_buf, fat_secs) != 0) return -1;
    static uint8_t dbuf[512];
    for (uint32_t s = 0; ; s++) {
        int r = read_dir_sector(s, dbuf, current_dir_cluster, fat_buf, fat_secs);
        if (r != 0) break;
        RawDirEntry* dir = (RawDirEntry*)dbuf;
        for (int i = 0; i < 512/32; i++) {
            uint8_t first = dir[i].name[0];
            if (first == 0x00) goto not_found;
            if (first == 0xE5) continue;
            if (!(dir[i].attributes & 0x10)) continue; 
            if (dir[i].name[0] == '.') continue;       
            char entry_name[FAT_NAME_LEN];
            make_name(dir[i].name, dir[i].ext, entry_name);
            if (k_strcmp_nocase(entry_name, name) == 0) {
                current_dir_cluster = dir[i].first_cluster;
                if (current_path[1] == '\0') {
                    char new_path[256];
                    new_path[0] = '/';
                    int j = 1;
                    for (int k = 0; entry_name[k]; k++) new_path[j++] = entry_name[k];
                    new_path[j] = '\0';
                    k_strcpy(current_path, new_path);
                } else {
                    int len = k_strlen(current_path);
                    current_path[len] = '/';
                    k_strcpy(current_path + len + 1, entry_name);
                }
                return 0;
            }
        }
    }
not_found:
    return -1;
}
int fat_rmdir(const char* name) {
    if (!initialized) return -1;
    static uint8_t fat_buf[18 * 512];
    uint32_t fat_secs = bpb.sectors_per_fat; if (fat_secs > 18) fat_secs = 18;
    if (load_fat(fat_buf, fat_secs) != 0) return -1;
    static uint8_t dbuf[512];
    for (uint32_t s = 0; ; s++) {
        int r = read_dir_sector(s, dbuf, current_dir_cluster, fat_buf, fat_secs);
        if (r != 0) break;
        RawDirEntry* dir = (RawDirEntry*)dbuf;
        int changed = 0;
        for (int i = 0; i < 512/32; i++) {
            uint8_t first = dir[i].name[0];
            if (first == 0x00) goto not_found2;
            if (first == 0xE5 || !(dir[i].attributes & 0x10)) continue;
            if (dir[i].name[0] == '.') continue;
            char entry_name[FAT_NAME_LEN];
            make_name(dir[i].name, dir[i].ext, entry_name);
            if (k_strcmp_nocase(entry_name, name) == 0) {
                uint16_t cluster = dir[i].first_cluster;
                while (cluster >= 2 && cluster <= 0xFEF) {
                    uint16_t next = fat_next_cluster(fat_buf, cluster);
                    fat_set_cluster(fat_buf, cluster, 0x000);
                    cluster = next;
                }
                write_fat(fat_buf, fat_secs);
                dir[i].name[0] = 0xE5;
                changed = 1; break;
            }
        }
        if (changed) {
            if (current_dir_cluster == 0) return ata_write(root_start + s, 1, dbuf);
            else {
                uint32_t spc = bpb.sectors_per_cluster;
                uint16_t cluster = (uint16_t)current_dir_cluster;
                for (uint32_t i = 0; i < s/spc; i++) cluster = fat_next_cluster(fat_buf, cluster);
                return ata_write(data_start + (uint32_t)(cluster-2)*spc + s%spc, 1, dbuf);
            }
        }
    }
not_found2:
    return -1;
}