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

# Flags de compilação
CFLAGS = -ffreestanding -m32 -Wall -Wextra -std=c99 -O2
LDFLAGS = -T $(KERNEL_DIR)/kernel.ld
ASMFLAGS = -f bin

# Arquivos fonte e binários
BOOTLOADER_SRC = $(BOOT_DIR)/bootloader.asm
BOOTLOADER_BIN = $(BUILD_DIR)/bootloader.bin

KERNEL_SRC = $(KERNEL_DIR)/kernel.c
KERNEL_OBJ = $(BUILD_DIR)/kernel.o
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

# Compilar o kernel objeto
$(KERNEL_OBJ): $(KERNEL_SRC) $(KERNEL_DIR)/kernel.h | $(BUILD_DIR)
	@echo "🔧 Compilando kernel..."
	$(CC) $(CFLAGS) -c $< -o $@

# Linkar o kernel ELF
$(KERNEL_ELF): $(KERNEL_OBJ)
	@echo "🔗 Linkando kernel..."
	$(LD) $(LDFLAGS) -o $@ $<

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


