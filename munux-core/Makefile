all: bootloader

bootloader:
	nasm -f bin bootloader.asm -o bootloader.bin

run:
	qemu-system-i386 -drive format=raw,file=bootloader.bin

clean:
	rm -f bootloader.bin

