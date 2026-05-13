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
RUST_DIR = $(KERNEL_DIR)/rust

# Rust build (v0.3+)
RUST_TARGET = i686-unknown-none
RUST_PROFILE = release
RUST_TARGET_DIR = $(RUST_DIR)/target/$(RUST_TARGET)/$(RUST_PROFILE)
RUST_LIB = $(RUST_TARGET_DIR)/libmunux_rs.a
RUST_HEADER = $(RUST_DIR)/include/munux_rs.h
RUST_SRCS = $(shell find $(RUST_DIR) -name '*.rs' -o -name 'Cargo.toml' -o -name '*.json' 2>/dev/null)

# Flags de compilação
# The -mno-{mmx,sse,*} block matches the Rust target spec: the kernel
# does not save FPU/SIMD state at context switches, so the compiler
# must never emit those instructions. Without these flags GCC -O2
# vectorizes memset/memcpy with SSE2, which triple-faults at boot.
CFLAGS = -ffreestanding -m32 -Wall -Wextra -std=c99 -O2 -nostdlib -fno-builtin -fno-stack-protector -fno-pic -fno-pie \
         -mno-mmx -mno-sse -mno-sse2 -mno-sse3 -mno-3dnow -mgeneral-regs-only \
         -I$(RUST_DIR)/include
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
.PHONY: all clean run debug test install qemu qemu-direct qemu-debug \
        rust rust-check rust-fmt rust-headers rust-clean

all: $(BUILD_DIR)/munux.img
	@echo "✅ Build concluído com sucesso!"
	@echo "💾 Imagem: $(BUILD_DIR)/munux.img"
	@echo ""
	@echo "Para testar, execute: make qemu"
	@echo "Para criar ISO (requer grub-mkrescue): make $(ISO_IMAGE)"

# Source-tree search paths so pattern rules find any .c or .asm
# regardless of which subdirectory holds it.
vpath %.c $(KERNEL_DIR) $(INTERRUPTS_DIR) $(MEMORY_DIR) $(PROCESS_DIR) $(DRIVERS_DIR)
vpath %.asm $(KERNEL_DIR) $(INTERRUPTS_DIR) $(PROCESS_DIR)

# Compilar o bootloader (flat binary, formato diferente do resto)
$(BOOTLOADER_BIN): $(BOOTLOADER_SRC) | $(BUILD_DIR)
	@echo "🔧 Compilando bootloader..."
	$(ASM) $(ASMFLAGS) $< -o $@

# Pattern rules — uma só regra cobre todos os .c e .asm do kernel.
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@echo "🔧 Compilando $<..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.asm | $(BUILD_DIR)
	@echo "🔧 Compilando $<..."
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

# Compilar a static library Rust (libmunux_rs.a)
$(RUST_LIB): $(RUST_SRCS)
	@echo "🦀 Compilando munux-rs (Rust)..."
	cd $(RUST_DIR) && cargo build --$(RUST_PROFILE)

# Atalhos para iterar só na parte Rust
rust: $(RUST_LIB)
	@echo "✅ libmunux_rs.a pronto: $(RUST_LIB)"

rust-check:
	@echo "🔎 cargo check + clippy..."
	cd $(RUST_DIR) && cargo check --lib
	cd $(RUST_DIR) && cargo clippy --lib -- -D warnings

rust-fmt:
	@echo "🎨 cargo fmt --check..."
	cd $(RUST_DIR) && cargo fmt --check

rust-headers:
	@echo "📝 Regenerando $(RUST_HEADER) com cbindgen..."
	cd $(RUST_DIR) && cbindgen --config cbindgen.toml --crate munux-rs-ffi --output include/munux_rs.h

rust-clean:
	@echo "🧹 cargo clean..."
	cd $(RUST_DIR) && cargo clean

# Linkar o kernel ELF (inclui libmunux_rs.a como peer dos .o)
$(KERNEL_ELF): $(KERNEL_OBJS) $(RUST_LIB)
	@echo "🔗 Linkando kernel (C + ASM + Rust)..."
	$(LD) $(LDFLAGS) -o $@ \
	    $(BUILD_DIR)/multiboot.o \
	    $(filter-out $(BUILD_DIR)/multiboot.o,$(KERNEL_OBJS)) \
	    $(RUST_LIB)

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
clean: rust-clean
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
	@command -v rustup >/dev/null 2>&1 || { echo "❌ rustup não encontrado (instale via https://rustup.rs)"; exit 1; }
	@command -v cargo >/dev/null 2>&1 || { echo "❌ cargo não encontrado"; exit 1; }
	@command -v cbindgen >/dev/null 2>&1 || { echo "⚠️  cbindgen não encontrado (necessário só para rust-headers)"; }
	@command -v grub-mkrescue >/dev/null 2>&1 || { echo "❌ grub-mkrescue não encontrado"; exit 1; }
	@command -v qemu-system-i386 >/dev/null 2>&1 || { echo "❌ qemu-system-i386 não encontrado"; exit 1; }
	@echo "✅ Todas as dependências estão disponíveis!"


