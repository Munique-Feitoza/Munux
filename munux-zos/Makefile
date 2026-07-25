# Makefile — Munux z/OS (s390x / z/Architecture)
#
# Kernel bare-metal em Rust (no_std) para s390x, bootável no qemu-system-s390x.

KERNEL_DIR = kernel
KERNEL_ELF = $(KERNEL_DIR)/target/s390x-unknown-none/release/munux-zos

.PHONY: all build qemu clean info

all: build
	@echo "✅ Kernel s390x pronto: $(KERNEL_ELF)"

build:
	cd $(KERNEL_DIR) && cargo build --release

# Boot no QEMU. Requer o emulador s390x:
#   sudo pacman -S qemu-system-s390x qemu-system-s390x-firmware
qemu: build
	qemu-system-s390x -M s390-ccw-virtio -m 128 -nographic -no-reboot -kernel $(KERNEL_ELF)

clean:
	cd $(KERNEL_DIR) && cargo clean

info:
	@echo "Alvo:   s390x-unknown-none (z/Architecture, 64-bit, big-endian)"
	@echo "Kernel: $(KERNEL_ELF)"
	@echo "Boot:   qemu-system-s390x -M s390-ccw-virtio -kernel ..."
