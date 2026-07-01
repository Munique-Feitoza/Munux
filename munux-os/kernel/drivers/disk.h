/*
 * Munux Kernel - ATA/IDE Disk Driver
 * 
 * Driver básico para discos ATA/IDE (Primary Master).
 */

#ifndef DISK_H
#define DISK_H

#include "../kernel.h"

// Tamanho do setor
#define SECTOR_SIZE 512

// Funções
void disk_init(void);
int disk_read_sector(uint32_t lba, uint8_t* buffer);
int disk_write_sector(uint32_t lba, const uint8_t* buffer);
void disk_identify(void);
int disk_present(void);

#endif // DISK_H
