#include "ata.h"
#include "common.h"
#include "port_io.h"
#include "string.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    ATA_PRIMARY_IO = 0x1F0,
    ATA_PRIMARY_CTRL = 0x3F6
};

static void ata_delay(void) {
    inb(ATA_PRIMARY_CTRL);
    inb(ATA_PRIMARY_CTRL);
    inb(ATA_PRIMARY_CTRL);
    inb(ATA_PRIMARY_CTRL);
}

static bool ata_wait_not_busy(void) {
    for (int i = 0; i < 100000; ++i) {
        if ((inb(ATA_PRIMARY_IO + 7) & 0x80) == 0) {
            return true;
        }
    }
    return false;
}

static bool ata_wait_drq(void) {
    for (int i = 0; i < 100000; ++i) {
        uint8_t status = inb(ATA_PRIMARY_IO + 7);
        if (status & 0x01) {
            return false;
        }
        if ((status & 0x08) != 0) {
            return true;
        }
    }
    return false;
}

bool ata_identify_primary(ata_identity_t *identity) {
    uint16_t buffer[256];

    if (!ata_wait_not_busy()) {
        return false;
    }

    outb(ATA_PRIMARY_IO + 6, 0xA0);
    ata_delay();
    outb(ATA_PRIMARY_IO + 2, 0x00);
    outb(ATA_PRIMARY_IO + 3, 0x00);
    outb(ATA_PRIMARY_IO + 4, 0x00);
    outb(ATA_PRIMARY_IO + 5, 0x00);
    outb(ATA_PRIMARY_IO + 7, 0xEC);

    if (inb(ATA_PRIMARY_IO + 7) == 0x00) {
        return false;
    }

    if (!ata_wait_not_busy()) {
        return false;
    }

    if (inb(ATA_PRIMARY_IO + 4) != 0 || inb(ATA_PRIMARY_IO + 5) != 0) {
        return false;
    }

    if (!ata_wait_drq()) {
        return false;
    }

    insw(ATA_PRIMARY_IO + 0, buffer, 256);

    if (identity != NULL) {
        memset(identity, 0, sizeof(*identity));
        for (int i = 0; i < 20; ++i) {
            identity->model[i * 2] = (char)(buffer[27 + i] >> 8);
            identity->model[i * 2 + 1] = (char)(buffer[27 + i] & 0xFF);
        }
        identity->model[40] = '\0';
        identity->lba28_sectors = ((uint32_t)buffer[61] << 16) | buffer[60];
    }

    return true;
}

bool ata_read28(uint32_t lba, uint8_t sector_count, void *buffer) {
    uint8_t *out = (uint8_t *)buffer;

    if (sector_count == 0 || lba > 0x0FFFFFFF) {
        return false;
    }

    if (!ata_wait_not_busy()) {
        return false;
    }

    outb(ATA_PRIMARY_IO + 6, (uint8_t)(0xE0 | ((lba >> 24) & 0x0F)));
    outb(ATA_PRIMARY_IO + 2, sector_count);
    outb(ATA_PRIMARY_IO + 3, (uint8_t)(lba & 0xFF));
    outb(ATA_PRIMARY_IO + 4, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_PRIMARY_IO + 5, (uint8_t)((lba >> 16) & 0xFF));
    outb(ATA_PRIMARY_IO + 7, 0x20);

    for (uint8_t sector = 0; sector < sector_count; ++sector) {
        if (!ata_wait_drq()) {
            return false;
        }
        insw(ATA_PRIMARY_IO + 0, out + sector * 512u, 256);
        ata_delay();
    }

    return true;
}
