# Makefile para Munux OS
# 
# Este Makefile compila o bootloader, kernel e gera uma imagem ISO
# bootável do sistema operacional Munux.

# Variáveis de configuração
CC = gcc
LD = ld
ASM = nasm
OBJCOPY = objcopy

# Diretórios
BOOT_DIR = boot
KERNEL_DIR = kernel
BUILD_DIR = build
INTERRUPTS_DIR = $(KERNEL_DIR)/interrupts
MEMORY_DIR = $(KERNEL_DIR)/memory
PROCESS_DIR = $(KERNEL_DIR)/process
DRIVERS_DIR = $(KERNEL_DIR)/drivers

# Flags de compilação
CFLAGS = -ffreestanding -m32 -Wall -Wextra -std=c99 -O2 -nostdlib -fno-builtin -fno-stack-protector -fno-pic -fno-pie
LDFLAGS = -m elf_i386 -T $(KERNEL_DIR)/kernel.ld -nostdlib
ASMFLAGS = -f bin
ASMFLAGS_ELF = -f elf32

# Arquivos fonte
BOOTLOADER_SRC = $(BOOT_DIR)/bootloader.asm
BOOTLOADER_BIN = $(BUILD_DIR)/bootloader.bin

# Kernel C files
KERNEL_C_SOURCES = $(KERNEL_DIR)/kernel.c \
                   $(INTERRUPTS_DIR)/idt.c \
                   $(MEMORY_DIR)/pmm.c \
                   $(MEMORY_DIR)/vmm.c \
                   $(MEMORY_DIR)/heap.c \
                   $(MEMORY_DIR)/utils.c \
                   $(PROCESS_DIR)/process.c \
                   $(PROCESS_DIR)/scheduler.c \
                   $(DRIVERS_DIR)/timer.c \
                   $(DRIVERS_DIR)/keyboard.c \
                   $(DRIVERS_DIR)/serial.c \
                   $(DRIVERS_DIR)/mouse.c \
                   $(DRIVERS_DIR)/disk.c

# Kernel ASM files
KERNEL_ASM_SOURCES = $(KERNEL_DIR)/multiboot.asm \
                     $(INTERRUPTS_DIR)/interrupt.asm \
                     $(PROCESS_DIR)/switch.asm

# Object files
KERNEL_C_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(notdir $(KERNEL_C_SOURCES)))
KERNEL_ASM_OBJS = $(patsubst %.asm,$(BUILD_DIR)/%.o,$(notdir $(KERNEL_ASM_SOURCES)))
KERNEL_OBJS = $(KERNEL_C_OBJS) $(KERNEL_ASM_OBJS)

KERNEL_ELF = $(BUILD_DIR)/kernel.elf
KERNEL_BIN = $(BUILD_DIR)/kernel.bin

ISO_IMAGE = $(BUILD_DIR)/munux.iso

# Targets principais
.PHONY: all clean run debug test install qemu qemu-direct qemu-debug

all: $(BUILD_DIR)/munux.img
	@echo "✅ Build concluído com sucesso!"
	@echo "💾 Imagem: $(BUILD_DIR)/munux.img"
	@echo ""
	@echo "Para testar, execute: make qemu"
	@echo "Para criar ISO (requer grub-mkrescue): make $(ISO_IMAGE)"

# Compilar o bootloader
$(BOOTLOADER_BIN): $(BOOTLOADER_SRC) | $(BUILD_DIR)
	@echo "🔧 Compilando bootloader..."
	$(ASM) $(ASMFLAGS) $< -o $@

# Compilar kernel C objects
$(BUILD_DIR)/kernel.o: $(KERNEL_DIR)/kernel.c | $(BUILD_DIR)
	@echo "🔧 Compilando $<..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/idt.o: $(INTERRUPTS_DIR)/idt.c | $(BUILD_DIR)
	@echo "🔧 Compilando $<..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/pmm.o: $(MEMORY_DIR)/pmm.c | $(BUILD_DIR)
	@echo "🔧 Compilando $<..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/vmm.o: $(MEMORY_DIR)/vmm.c | $(BUILD_DIR)
	@echo "🔧 Compilando $<..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/heap.o: $(MEMORY_DIR)/heap.c | $(BUILD_DIR)
	@echo "🔧 Compilando $<..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/utils.o: $(MEMORY_DIR)/utils.c | $(BUILD_DIR)
	@echo "🔧 Compilando $<..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/process.o: $(PROCESS_DIR)/process.c | $(BUILD_DIR)
	@echo "🔧 Compilando $<..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/scheduler.o: $(PROCESS_DIR)/scheduler.c | $(BUILD_DIR)
	@echo "🔧 Compilando $<..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/timer.o: $(DRIVERS_DIR)/timer.c | $(BUILD_DIR)
	@echo "🔧 Compilando $<..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/keyboard.o: $(DRIVERS_DIR)/keyboard.c | $(BUILD_DIR)
	@echo "🔧 Compilando $<..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/serial.o: $(DRIVERS_DIR)/serial.c | $(BUILD_DIR)
	@echo "🔧 Compilando $<..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mouse.o: $(DRIVERS_DIR)/mouse.c | $(BUILD_DIR)
	@echo "🔧 Compilando $<..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/disk.o: $(DRIVERS_DIR)/disk.c | $(BUILD_DIR)
	@echo "🔧 Compilando $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Compilar kernel ASM objects
$(BUILD_DIR)/multiboot.o: $(KERNEL_DIR)/multiboot.asm | $(BUILD_DIR)
	@echo "🔧 Compilando $<..."
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

$(BUILD_DIR)/interrupt.o: $(INTERRUPTS_DIR)/interrupt.asm | $(BUILD_DIR)
	@echo "🔧 Compilando $<..."
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

$(BUILD_DIR)/switch.o: $(PROCESS_DIR)/switch.asm | $(BUILD_DIR)
	@echo "🔧 Compilando $<..."
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

# Linkar o kernel ELF
$(KERNEL_ELF): $(KERNEL_OBJS)
	@echo "🔗 Linkando kernel..."
	$(LD) $(LDFLAGS) -o $@ $(BUILD_DIR)/multiboot.o $(filter-out $(BUILD_DIR)/multiboot.o,$(KERNEL_OBJS))

# Converter kernel ELF para binário
$(KERNEL_BIN): $(KERNEL_ELF)
	@echo "📦 Gerando binário do kernel..."
	$(OBJCOPY) -O binary $< $@

# Gerar imagem ISO
$(ISO_IMAGE): $(KERNEL_ELF) | $(BUILD_DIR)/iso/boot/grub
	@echo "💿 Criando imagem ISO..."
	cp $(KERNEL_ELF) $(BUILD_DIR)/iso/boot/kernel.bin
	cp boot/grub/grub.cfg $(BUILD_DIR)/iso/boot/grub/grub.cfg
	@which grub-mkrescue > /dev/null 2>&1 || (echo "❌ grub-mkrescue não encontrado." && exit 1)
	grub-mkrescue -o $@ $(BUILD_DIR)/iso 2>/dev/null || (echo "❌ Erro ao criar ISO" && exit 1)
	@echo "✅ ISO criado: $(ISO_IMAGE)"

# Criar imagem de disco raw (alternativa ao ISO)
$(BUILD_DIR)/munux.img: $(BOOTLOADER_BIN) $(KERNEL_BIN)
	@echo "💾 Criando imagem de disco..."
	dd if=/dev/zero of=$@ bs=512 count=2880 2>/dev/null
	dd if=$(BOOTLOADER_BIN) of=$@ conv=notrunc 2>/dev/null
	dd if=$(KERNEL_BIN) of=$@ seek=1 conv=notrunc 2>/dev/null
	@echo "✅ Imagem criada: $@"

# Criar diretórios necessários
$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/iso/boot/grub: | $(BUILD_DIR)
	mkdir -p $@

# Executar no QEMU
run: $(ISO_IMAGE)
	@echo "🚀 Executando Munux no QEMU..."
	qemu-system-i386 -cdrom $< -m 32M -serial stdio

# Executar diretamente com imagem raw (sem precisar de GRUB)
qemu-direct: $(BUILD_DIR)/munux.img
	@echo "🚀 Executando Munux no QEMU (modo direto)..."
	qemu-system-i386 -drive file=$<,format=raw,if=floppy -m 32M -serial stdio -d cpu_reset,guest_errors

# Atalho para QEMU
qemu: qemu-direct

# Executar no QEMU com debug
debug: $(ISO_IMAGE)
	@echo "🐛 Executando Munux no QEMU (modo debug)..."
	qemu-system-i386 -cdrom $< -m 32M -s -S

# Debug modo direto
qemu-debug: $(BUILD_DIR)/munux.img
	@echo "🐛 Executando Munux no QEMU (modo debug direto)..."
	qemu-system-i386 -drive format=raw,file=$< -m 32M -s -S

# Executar testes básicos
# Executar no QEMU (direto do kernel ELF - não precisa de ISO)
qemu-kernel: $(KERNEL_ELF)
	@echo "🚀 Iniciando Munux no QEMU (boot direto do kernel)..."
	qemu-system-i386 -kernel $(KERNEL_ELF) -m 32M

# Executar no QEMU a partir da ISO
qemu-iso: $(ISO_IMAGE)
	@echo "🚀 Iniciando Munux no QEMU (boot da ISO)..."
	qemu-system-i386 -cdrom $(ISO_IMAGE) -m 32M

test: $(ISO_IMAGE)
	@echo "🧪 Executando testes..."
	@echo "Verificando se a imagem ISO foi criada..."
	@test -f $(ISO_IMAGE) && echo "✅ ISO existe" || echo "❌ ISO não encontrado"
	@echo "Verificando tamanho da imagem..."
	@ls -lh $(ISO_IMAGE)

# Limpar arquivos gerados
clean:
	@echo "🧹 Limpando arquivos gerados..."
	rm -rf $(BUILD_DIR)
	@echo "✅ Limpeza concluída!"

# Informações sobre o build
info:
	@echo "📋 Informações do Munux Build System"
	@echo "======================================"
	@echo "Compilador C: $(CC)"
	@echo "Linker: $(LD)"
	@echo "Assembler: $(ASM)"
	@echo "Flags C: $(CFLAGS)"
	@echo "Flags LD: $(LDFLAGS)"
	@echo "Diretório build: $(BUILD_DIR)"
	@echo "======================================"

# Verificar dependências
check-deps:
	@echo "🔍 Verificando dependências..."
	@command -v $(CC) >/dev/null 2>&1 || { echo "❌ $(CC) não encontrado"; exit 1; }
	@command -v $(LD) >/dev/null 2>&1 || { echo "❌ $(LD) não encontrado"; exit 1; }
	@command -v $(ASM) >/dev/null 2>&1 || { echo "❌ $(ASM) não encontrado"; exit 1; }
	@command -v grub-mkrescue >/dev/null 2>&1 || { echo "❌ grub-mkrescue não encontrado"; exit 1; }
	@command -v qemu-system-i386 >/dev/null 2>&1 || { echo "❌ qemu-system-i386 não encontrado"; exit 1; }
	@echo "✅ Todas as dependências estão disponíveis!"
	rm -rf $(BUILD_DIR)


