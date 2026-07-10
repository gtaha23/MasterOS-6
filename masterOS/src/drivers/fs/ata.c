#include "ata.h"
#include "../../memory/util.h"

// ── ATA Primary Bus Ports ─────────────────────────────────────────────────────
#define ATA_DATA         0x1F0
#define ATA_ERROR        0x1F1
#define ATA_SECTOR_COUNT 0x1F2
#define ATA_LBA_LOW      0x1F3
#define ATA_LBA_MID      0x1F4
#define ATA_LBA_HIGH     0x1F5
#define ATA_DRIVE_HEAD   0x1F6
#define ATA_STATUS       0x1F7
#define ATA_COMMAND      0x1F7

// ── Status bits ───────────────────────────────────────────────────────────────
#define ATA_SR_BSY  0x80
#define ATA_SR_DRQ  0x08
#define ATA_SR_ERR  0x01
#define ATA_SR_RDY  0x40

// ── Drive select ──────────────────────────────────────────────────────────────
#define ATA_MASTER 0xE0
#define ATA_SLAVE  0xF0

// ── Commands ──────────────────────────────────────────────────────────────────
#define ATA_CMD_READ_PIO     0x20
#define ATA_CMD_WRITE_PIO    0x30
#define ATA_CMD_CACHE_FLUSH  0xE7

// Which drive we found — set once by ata_init()
static uint8_t ata_drive = ATA_SLAVE; // default to slave since CD is master

// ── Helpers ───────────────────────────────────────────────────────────────────

static inline uint16_t ata_read_word() {
    return inPortW(ATA_DATA);
}

static int ata_wait_bsy() {
    int timeout = 100000;
    while ((inPortB(ATA_STATUS) & ATA_SR_BSY) && timeout--);
    return (timeout > 0) ? 0 : -1;
}

static int ata_wait_drq() {
    int timeout = 100000;
    uint8_t status;
    while (timeout--) {
        status = inPortB(ATA_STATUS);
        if (status & ATA_SR_ERR) return -1;
        if (status & ATA_SR_DRQ) return 0;
    }
    return -1;
}

// Check if a drive exists by selecting it and reading status
static int ata_probe(uint8_t drive) {
    outPortB(ATA_DRIVE_HEAD, drive);
    // Small delay — read status port 4 times (standard ATA delay)
    for (int i = 0; i < 4; i++) inPortB(ATA_STATUS);
    uint8_t status = inPortB(ATA_STATUS);
    // 0xFF means no drive, 0x00 also bad
    if (status == 0xFF || status == 0x00) return -1;
    return 0;
}

// ── Init — auto-detect master or slave ───────────────────────────────────────

int ata_init() {
    // CD-ROM is on master (index 0), so try slave first
    if (ata_probe(ATA_SLAVE) == 0) {
        ata_drive = ATA_SLAVE;
        return 0;
    }
    // Fall back to master
    if (ata_probe(ATA_MASTER) == 0) {
        ata_drive = ATA_MASTER;
        return 0;
    }
    return -1; // no drive found
}

// ── Public read ───────────────────────────────────────────────────────────────

int ata_read(uint32_t lba, uint8_t count, uint8_t* buf) {
    if (ata_wait_bsy() != 0) return -1;

    outPortB(ATA_DRIVE_HEAD,   ata_drive | ((lba >> 24) & 0x0F));
    outPortB(ATA_ERROR,        0x00);
    outPortB(ATA_SECTOR_COUNT, count);
    outPortB(ATA_LBA_LOW,      (uint8_t)(lba));
    outPortB(ATA_LBA_MID,      (uint8_t)(lba >> 8));
    outPortB(ATA_LBA_HIGH,     (uint8_t)(lba >> 16));
    outPortB(ATA_COMMAND,      ATA_CMD_READ_PIO);

    for (uint8_t s = 0; s < count; s++) {
        if (ata_wait_bsy() != 0) return -1;
        if (ata_wait_drq() != 0) return -1;

        uint16_t* ptr = (uint16_t*)(buf + s * 512);
        for (int i = 0; i < 256; i++) {
            ptr[i] = ata_read_word();
        }
    }

    return 0;
}

// ── Public write ──────────────────────────────────────────────────────────────

int ata_write(uint32_t lba, uint8_t count, const uint8_t* buf) {
    if (ata_wait_bsy() != 0) return -1;

    outPortB(ATA_DRIVE_HEAD,   ata_drive | ((lba >> 24) & 0x0F));
    outPortB(ATA_ERROR,        0x00);
    outPortB(ATA_SECTOR_COUNT, count);
    outPortB(ATA_LBA_LOW,      (uint8_t)(lba));
    outPortB(ATA_LBA_MID,      (uint8_t)(lba >> 8));
    outPortB(ATA_LBA_HIGH,     (uint8_t)(lba >> 16));
    outPortB(ATA_COMMAND,      ATA_CMD_WRITE_PIO);

    for (uint8_t s = 0; s < count; s++) {
        if (ata_wait_bsy() != 0) return -1;
        if (ata_wait_drq() != 0) return -1;

        const uint16_t* ptr = (const uint16_t*)(buf + s * 512);
        for (int i = 0; i < 256; i++) {
            outPortW(ATA_DATA, ptr[i]);
        }

        // Flush cache after each sector write (standard practice)
        outPortB(ATA_COMMAND, ATA_CMD_CACHE_FLUSH);
        if (ata_wait_bsy() != 0) return -1;
    }

    return 0;
}
