# Variáveis
BOOT_DIR = boot
KERNEL_DIR = kernel
BUILD_DIR = build

BOOTLOADER_SRC = $(BOOT_DIR)/bootloader.asm
BOOTLOADER_BIN = $(BUILD_DIR)/bootloader.bin
PMODE_SRC = $(BOOT_DIR)/pmode.asm
PMODE_BIN = $(BUILD_DIR)/pmode.bin
KERNEL_SRC = $(KERNEL_DIR)/kernel.c
KERNEL_OBJ = $(BUILD_DIR)/kernel.o
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
ISO_IMAGE = $(BUILD_DIR)/munux.iso

# Targets principais
all: $(ISO_IMAGE)

# Compilar o bootloader
$(BOOTLOADER_BIN): $(BOOTLOADER_SRC)
	mkdir -p $(BUILD_DIR)
	nasm -f bin $< -o $@
	
# Compilar o modo protegido
$(PMODE_BIN): $(PMODE_SRC)
	mkdir -p $(BUILD_DIR)
	nasm -f bin $< -o $@

# Compilar o kernel
$(KERNEL_OBJ): $(KERNEL_SRC)
	mkdir -p $(BUILD_DIR)
	i686-elf-gcc -ffreestanding -m32 -c $< -o $@

$(KERNEL_BIN): $(KERNEL_OBJ)
	i686-elf-ld -T $(KERNEL_DIR)/kernel.ld -o $(BUILD_DIR)/kernel.elf $(KERNEL_OBJ)
	objcopy -O binary $(BUILD_DIR)/kernel.elf $(KERNEL_BIN)

# Gerar ISO
$(ISO_IMAGE): $(BOOTLOADER_BIN) $(PMODE_BIN) $(KERNEL_BIN)
	mkdir -p $(BUILD_DIR)/iso/boot
	cp $(BOOTLOADER_BIN) $(BUILD_DIR)/iso/boot/bootloader.bin
	cp $(PMODE_BIN) $(BUILD_DIR)/iso/boot/pmode.bin
	cp $(KERNEL_BIN) $(BUILD_DIR)/iso/boot/kernel.bin
	grub-mkrescue -o $@ $(BUILD_DIR)/iso

# Rodar no QEMU
run: $(ISO_IMAGE)
	qemu-system-i386 -cdrom $<

# Limpar arquivos gerados
clean:
	rm -rf $(BUILD_DIR)


