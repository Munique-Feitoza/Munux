# Device Drivers

## Overview

Device drivers provide abstraction layers between hardware and kernel subsystems. Each driver encapsulates device-specific details behind a clean, consistent interface.

## Timer Driver (PIT)

### Programmable Interval Timer

The 8253/8254 PIT generates periodic interrupts for timekeeping and scheduling. The chip contains three independent counters, but we use only channel 0.

### Configuration

The PIT oscillator runs at 1.193182 MHz. To generate interrupts at a desired frequency:

1. Calculate divisor = 1193182 / desired_frequency
2. Send mode/command byte to port 0x43
3. Send low byte of divisor to port 0x40
4. Send high byte of divisor to port 0x40

The mode byte configures channel 0 for rate generator mode (mode 2), which produces square wave output suitable for interrupts.

### Initialization

Timer initialization:

```c
void timer_init(uint32_t frequency) {
    register_interrupt_handler(32, timer_callback);
    
    uint32_t divisor = 1193182 / frequency;
    outb(0x43, 0x36);  // Command: channel 0, lobyte/hibyte, mode 2
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
}
```

We typically configure 100 Hz (10ms period) for reasonable scheduling granularity without excessive overhead.

### Interrupt Handler

The timer handler increments a tick counter on each interrupt:

```c
static volatile uint32_t timer_ticks = 0;

static void timer_callback(struct registers* regs) {
    timer_ticks++;
}
```

The volatile qualifier ensures the compiler doesn't optimize away accesses to this shared variable.

### Timekeeping

Higher-level timing functions use the tick counter:

**Get Current Time**: Simply return the tick count

**Sleep/Wait**: Loop until ticks reaches target value, halting CPU between checks

**Elapsed Time**: Subtract start tick count from current count

For precise timing, multiple PIT channels or other timers (HPET, TSC) can be used.

## Keyboard Driver

### PS/2 Controller

The keyboard connects via the PS/2 controller, a legacy interface still emulated on modern systems. The controller has two ports (keyboard and mouse) sharing interrupt lines.

### Scancodes

When a key is pressed or released, the keyboard sends a scancode byte. Different keyboard layouts use different scancode sets, but we assume Set 1 (most common).

Each key has:
- Make code: Sent when key is pressed  
- Break code: Sent when key is released (make code | 0x80)

Special keys send multi-byte sequences, but we implement single-byte handling for simplicity.

### Scancode Translation

The driver includes lookup tables mapping scancodes to ASCII characters:

```c
static const char scancode_to_ascii[] = {
    0, ESC, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', ...
};

static const char scancode_to_ascii_shift[] = {
    0, ESC, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', ...
};
```

Two tables handle unshifted and shifted characters.

### Modifier Keys

Special handling for modifier keys:

**Shift**: Left/right shift set a flag affecting character translation

**Ctrl**: Sets flag but character translation not yet implemented

**Alt**: Sets flag for future use

**Caps Lock**: Toggles flag on press (not release), affects letter case

The modifier state persists across key presses until changed.

### Input Buffer

A circular buffer stores incoming characters:

```c
static char keyboard_buffer[256];
static volatile uint32_t buffer_read_pos = 0;
static volatile uint32_t buffer_write_pos = 0;
```

The interrupt handler writes characters to the buffer. Application code reads from it. The circular structure prevents buffer overflow by wrapping indices modulo buffer size.

### Interrupt Handler

On IRQ1:

1. Read scancode from port 0x60
2. Check high bit for press/release
3. Update modifier state if modifier key
4. Translate scancode to ASCII using current modifier state
5. Add character to circular buffer

Special keys (arrows, function keys) use extended codes above 127.

### Public Interface

Applications access the keyboard through:

**keyboard_getchar()**: Blocking read of next character

**keyboard_available()**: Check if characters are buffered

**keyboard_flush()**: Clear buffer

**keyboard_get_state()**: Query modifier key states

This abstraction hides hardware details from higher-level code.

## Mouse Driver

### PS/2 Mouse Protocol

The mouse sends three-byte packets containing:
- Byte 0: Button states and movement sign bits
- Byte 1: X movement delta (-256 to +255)
- Byte 2: Y movement delta (-256 to +255)

Extensions exist for scroll wheels and extra buttons, but we implement basic three-button support.

### Initialization

Mouse initialization is more complex than keyboard:

1. Enable auxiliary device on PS/2 controller
2. Configure controller to route IRQ12
3. Send "set defaults" command to mouse
4. Send "enable data reporting" command
5. Wait for acknowledgment after each command

The mouse controller uses separate command and data ports from the keyboard.

### Packet Assembly

The interrupt handler assembles complete packets:

```c
static uint8_t mouse_cycle = 0;
static int8_t mouse_byte[3];

static void mouse_handler(struct registers* regs) {
    mouse_byte[mouse_cycle++] = inb(0x60);
    
    if (mouse_cycle == 3) {
        mouse_cycle = 0;
        process_packet(mouse_byte);
    }
}
```

Only when all three bytes arrive do we process the packet.

### Movement Processing

To extract movement deltas:

1. Read byte 1 and byte 2
2. Check status bits for sign extension
3. If sign bit set, extend to negative 32-bit value
4. Invert Y axis (PS/2 has inverted Y)
5. Update absolute position
6. Extract button states from byte 0
7. Call registered callback with delta and buttons

Callbacks allow higher-level code (GUI, cursor) to respond to mouse events without polling.

### Cursor Management

Future GUI code will use mouse input to:
- Update on-screen cursor position
- Detect clicks on UI elements  
- Implement drag-and-drop
- Handle scroll events

## Serial Port Driver

### RS-232 Communication

Serial ports provide asynchronous communication with external devices. While largely replaced by USB, serial is still useful for:
- Debugging via serial console
- Embedded device communication
- Legacy hardware support

### UART Registers

Each serial port (COM1-COM4) has a set of I/O ports:

**Data Register (0x3F8)**: Transmit/receive bytes

**Interrupt Enable (0x3F9)**: Enable interrupt generation

**Interrupt ID (0x3FA)**: Identify interrupt source

**Line Control (0x3FB)**: Configure data format

**Modem Control (0x3FC)**: Control modem signals

**Line Status (0x3FD)**: Check transmit/receive status

**Modem Status (0x3FE)**: Read modem signals

**Scratch (0x3FF)**: Unused scratchpad register

### Initialization

Serial port setup:

1. Disable interrupts
2. Enable DLAB (Divisor Latch Access Bit)
3. Set baud rate divisor (38400 baud typical)
4. Configure 8N1 (8 bits, no parity, 1 stop bit)
5. Enable and clear FIFO
6. Set modem control signals
7. Test with loopback
8. Set normal operational mode

After initialization, the port is ready for transmit/receive.

### Transmission

To send a byte:

1. Wait for transmit buffer empty (line status bit 5)
2. Write byte to data register
3. Byte is automatically transmitted

For strings, iterate and send each character.

### Reception

To receive a byte:

1. Wait for data ready (line status bit 0)
2. Read byte from data register

The FIFO buffers incoming data, reducing chance of overruns.

### Debug Output

Serial ports are invaluable for kernel debugging:

```c
serial_writestring(COM1, "Debug: entering function\n");
```

Unlike video output, serial continues working even when video is corrupted or the system is in a weird state.

## Disk Driver (ATA/IDE)

### ATA Interface

ATA (AT Attachment) provides a standard interface for storage devices. Modern SATA is backwards-compatible with ATA for basic operations.

We implement PIO (Programmed I/O) mode where the CPU directly reads/writes disk sectors. DMA mode would be more efficient but more complex.

### Register Set

Primary IDE controller uses ports 0x1F0-0x1F7:

**0x1F0 (Data)**: 16-bit data transfers

**0x1F1 (Error)**: Error information on failed commands

**0x1F2 (Sector Count)**: Number of sectors to transfer

**0x1F3-0x1F5 (LBA Low/Mid/High)**: Logical Block Address

**0x1F6 (Drive/Head)**: Drive select and LBA high bits

**0x1F7 (Status/Command)**: Read status, write commands

### Initialization

Disk initialization:

1. Select master drive (0xA0 to drive/head register)
2. Reset sector count and LBA to zero
3. Optionally send IDENTIFY command to detect drive

The IDENTIFY command returns 256 words of information including model, serial number, and capacity.

### Reading Sectors

To read a sector:

1. Wait for drive not busy
2. Select drive and LBA mode
3. Write sector count (typically 1)
4. Write LBA address (28-bit addressing)
5. Send READ SECTORS command
6. Wait for drive ready
7. Wait for data request (DRQ)
8. Read 256 words (512 bytes) from data port

Each word is 16 bits, so two bytes per inw() call.

### Writing Sectors

Writing is similar:

1. Wait for drive not busy
2. Select drive and LBA mode
3. Write sector count and LBA
4. Send WRITE SECTORS command
5. Wait for drive ready and DRQ
6. Write 256 words to data port
7. Wait for completion

After writing, the drive may need time to physically write to media.

### Error Handling

After commands, check the error register and status flags:
- ERR bit indicates command failed
- Error register contains specific error code
- Timeout waiting for DRQ suggests hardware problem

Proper error handling allows graceful degradation when disks fail.

### Future Enhancements

Several improvements are possible:

**DMA Mode**: Let disk controller transfer data directly to memory

**48-bit LBA**: Support disks larger than 128GB

**ATAPI**: Support CD/DVD drives using ATA packet interface

**Multiple Drives**: Detect and manage master/slave drives

**NCQ**: Native Command Queuing for better performance

---

**Implementation Files**:
- `kernel/drivers/timer.c/.h` - Programmable Interval Timer
- `kernel/drivers/keyboard.c/.h` - PS/2 keyboard driver
- `kernel/drivers/mouse.c/.h` - PS/2 mouse driver
- `kernel/drivers/serial.c/.h` - RS-232 serial port
- `kernel/drivers/disk.c/.h` - ATA/IDE disk controller
