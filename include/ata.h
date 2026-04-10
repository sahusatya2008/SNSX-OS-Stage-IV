#ifndef ATA_H
#define ATA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct ata_identity {
    char model[41];
    uint32_t lba28_sectors;
} ata_identity_t;

bool ata_identify_primary(ata_identity_t *identity);
bool ata_read28(uint32_t lba, uint8_t sector_count, void *buffer);

#endif
