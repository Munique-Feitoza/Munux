/*
 * Munux Kernel - ATA/IDE Disk Driver Implementation
 */

#include "disk.h"
#include "../interrupts/io.h"

// Portas ATA Primary
#define ATA_PRIMARY_DATA       0x1F0
#define ATA_PRIMARY_ERROR      0x1F1
#define ATA_PRIMARY_SECCOUNT   0x1F2
#define ATA_PRIMARY_LBA_LOW    0x1F3
#define ATA_PRIMARY_LBA_MID    0x1F4
#define ATA_PRIMARY_LBA_HIGH   0x1F5
#define ATA_PRIMARY_DRIVE      0x1F6
#define ATA_PRIMARY_STATUS     0x1F7
#define ATA_PRIMARY_COMMAND    0x1F7

// Status bits
#define ATA_SR_BSY  0x80
#define ATA_SR_DRDY 0x40
#define ATA_SR_DRQ  0x08
#define ATA_SR_ERR  0x01

// Comandos
#define ATA_CMD_READ_SECTORS  0x20
#define ATA_CMD_WRITE_SECTORS 0x30
#define ATA_CMD_IDENTIFY      0xEC

// Aguarda disco ficar pronto
static void ata_wait_bsy(void) {
    while (inb(ATA_PRIMARY_STATUS) & ATA_SR_BSY);
}

// Aguarda disco pronto para transferência
static void ata_wait_drq(void) {
    while (!(inb(ATA_PRIMARY_STATUS) & ATA_SR_DRQ));
}

// Inicializa disco
void disk_init(void) {
    // Seleciona drive master
    outb(ATA_PRIMARY_DRIVE, 0xA0);
    io_wait();
    
    // Reseta contador de setores
    outb(ATA_PRIMARY_SECCOUNT, 0);
    outb(ATA_PRIMARY_LBA_LOW, 0);
    outb(ATA_PRIMARY_LBA_MID, 0);
    outb(ATA_PRIMARY_LBA_HIGH, 0);
}

// Lê setor do disco
int disk_read_sector(uint32_t lba, uint8_t* buffer) {
    ata_wait_bsy();
    
    // Seleciona drive e modo LBA
    outb(ATA_PRIMARY_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_SECCOUNT, 1);
    outb(ATA_PRIMARY_LBA_LOW, (uint8_t)lba);
    outb(ATA_PRIMARY_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_PRIMARY_LBA_HIGH, (uint8_t)(lba >> 16));
    outb(ATA_PRIMARY_COMMAND, ATA_CMD_READ_SECTORS);
    
    ata_wait_bsy();
    ata_wait_drq();
    
    // Lê dados (256 words = 512 bytes)
    for (int i = 0; i < 256; i++) {
        uint16_t data = inw(ATA_PRIMARY_DATA);
        buffer[i * 2] = (uint8_t)data;
        buffer[i * 2 + 1] = (uint8_t)(data >> 8);
    }
    
    return 0;
}

// Escreve setor no disco
int disk_write_sector(uint32_t lba, const uint8_t* buffer) {
    ata_wait_bsy();
    
    // Seleciona drive e modo LBA
    outb(ATA_PRIMARY_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_SECCOUNT, 1);
    outb(ATA_PRIMARY_LBA_LOW, (uint8_t)lba);
    outb(ATA_PRIMARY_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_PRIMARY_LBA_HIGH, (uint8_t)(lba >> 16));
    outb(ATA_PRIMARY_COMMAND, ATA_CMD_WRITE_SECTORS);
    
    ata_wait_bsy();
    ata_wait_drq();
    
    // Escreve dados (256 words = 512 bytes)
    for (int i = 0; i < 256; i++) {
        uint16_t data = buffer[i * 2] | (buffer[i * 2 + 1] << 8);
        outw(ATA_PRIMARY_DATA, data);
    }
    
    // Flush cache
    ata_wait_bsy();
    
    return 0;
}

// Identifica disco. Os 256 words retornados pelo IDENTIFY são lidos e
// descartados — a leitura é necessária para esvaziar o buffer do
// controlador, mas o kernel ainda não consome esses metadados.
void disk_identify(void) {
    outb(ATA_PRIMARY_DRIVE, 0xA0);
    outb(ATA_PRIMARY_SECCOUNT, 0);
    outb(ATA_PRIMARY_LBA_LOW, 0);
    outb(ATA_PRIMARY_LBA_MID, 0);
    outb(ATA_PRIMARY_LBA_HIGH, 0);
    outb(ATA_PRIMARY_COMMAND, ATA_CMD_IDENTIFY);

    if (inb(ATA_PRIMARY_STATUS) == 0) {
        return; // Nenhum drive
    }

    ata_wait_bsy();
    for (int i = 0; i < 256; i++) {
        (void)inw(ATA_PRIMARY_DATA);
    }
}
