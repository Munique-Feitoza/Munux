# Makefile para Munux OS
# 
# Este Makefile compila o bootloader, kernel e gera uma imagem ISO
# bootável do sistema operacional Munux.

# Variáveis de configuração
CC = i686-elf-gcc
LD = i686-elf-ld
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
CFLAGS = -ffreestanding -m32 -Wall -Wextra -std=c99 -O2 -nostdlib -fno-builtin -fno-stack-protector
LDFLAGS = -T $(KERNEL_DIR)/kernel.ld -nostdlib
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
KERNEL_ASM_SOURCES = $(INTERRUPTS_DIR)/interrupt.asm \
                     $(PROCESS_DIR)/switch.asm

# Object files
KERNEL_C_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(notdir $(KERNEL_C_SOURCES)))
KERNEL_ASM_OBJS = $(patsubst %.asm,$(BUILD_DIR)/%.o,$(notdir $(KERNEL_ASM_SOURCES)))
KERNEL_OBJS = $(KERNEL_C_OBJS) $(KERNEL_ASM_OBJS)

KERNEL_ELF = $(BUILD_DIR)/kernel.elf
KERNEL_BIN = $(BUILD_DIR)/kernel.bin

ISO_IMAGE = $(BUILD_DIR)/munux.iso

# Targets principais
.PHONY: all clean run debug test install

all: $(ISO_IMAGE)
	@echo "✅ Build concluído com sucesso!"
	@echo "📀 Imagem ISO: $(ISO_IMAGE)"

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
$(BUILD_DIR)/interrupt.o: $(INTERRUPTS_DIR)/interrupt.asm | $(BUILD_DIR)
	@echo "🔧 Compilando $<..."
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

$(BUILD_DIR)/switch.o: $(PROCESS_DIR)/switch.asm | $(BUILD_DIR)
	@echo "🔧 Compilando $<..."
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

# Linkar o kernel ELF
$(KERNEL_ELF): $(KERNEL_OBJS)
	@echo "🔗 Linkando kernel..."
	$(LD) $(LDFLAGS) -o $@ $^

# Converter kernel ELF para binário
$(KERNEL_BIN): $(KERNEL_ELF)
	@echo "📦 Gerando binário do kernel..."
	$(OBJCOPY) -O binary $< $@

# Gerar imagem ISO
$(ISO_IMAGE): $(BOOTLOADER_BIN) $(KERNEL_BIN) | $(BUILD_DIR)/iso/boot/grub
	@echo "💿 Criando imagem ISO..."
	cp $(KERNEL_BIN) $(BUILD_DIR)/iso/boot/
	grub-mkrescue -o $@ $(BUILD_DIR)/iso 2>/dev/null
	@echo "✅ ISO criado: $(ISO_IMAGE)"

# Criar diretórios necessários
$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/iso/boot/grub: | $(BUILD_DIR)
	mkdir -p $@

# Executar no QEMU
run: $(ISO_IMAGE)
	@echo "🚀 Executando Munux no QEMU..."
	qemu-system-i386 -cdrom $< -m 32M

# Executar no QEMU com debug
debug: $(ISO_IMAGE)
	@echo "🐛 Executando Munux no QEMU (modo debug)..."
	qemu-system-i386 -cdrom $< -m 32M -s -S

# Executar testes básicos
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


