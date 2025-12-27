# Memory Management Subsystem

## Overview

The Munux memory management subsystem provides three distinct layers of abstraction: physical frame allocation, virtual memory paging, and heap allocation. This layered approach separates concerns while providing efficient memory utilization.

## Physical Memory Manager (PMM)

### Design Philosophy

The PMM operates at the lowest level, managing physical RAM as discrete 4KB frames. By working with fixed-size chunks, the allocator avoids fragmentation issues that plague variable-size allocators.

### Bitmap Implementation

A bitmap tracks the allocation status of every frame in the system. Each bit represents one 4KB frame:
- Bit clear (0) indicates the frame is available for allocation
- Bit set (1) indicates the frame is currently in use

This compact representation requires only one byte per 32KB of RAM, making it highly space-efficient even for large memory configurations.

The bitmap itself resides at physical address 0x100000 (1MB mark), placing it above the kernel but below general-purpose memory. This location avoids conflicts with legacy BIOS regions while keeping the bitmap easily accessible.

### Allocation Algorithm

Frame allocation proceeds through a linear scan of the bitmap:

1. Iterate through each 32-bit word of the bitmap
2. When a word is not 0xFFFFFFFF (all frames used), examine individual bits
3. Find the first clear bit within the word
4. Mark the bit as set and return the corresponding physical address
5. Increment the used frame counter

This approach has O(n) worst-case complexity where n is the number of frames, but performs well in practice because most allocations find free frames quickly.

### Deallocation

Freeing a frame is O(1):

1. Calculate the frame number by dividing the address by 4096
2. Calculate the word index and bit offset
3. Clear the corresponding bit
4. Decrement the used frame counter

### Initialization

During system startup, the PMM:

1. Calculates total frames based on available RAM
2. Zeros the entire bitmap
3. Marks the first 2MB as used (kernel and bitmap)
4. Records total and available frame counts

The assumption of 32MB total RAM can be replaced with BIOS memory detection in future versions.

### Memory Accounting

The PMM maintains counters for total and used frames, allowing the system to query available memory at any time. This information is useful for:
- Displaying memory usage to users
- Making allocation decisions
- Detecting low memory conditions

## Virtual Memory Manager (VMM)

### Paging Fundamentals

The VMM implements hardware paging to provide memory protection and virtual address spaces. Each process can access the full 4GB virtual address space, even though physical RAM is much smaller.

### Two-Level Page Tables

The x86 architecture uses a two-level paging structure:

**Page Directory**: Contains 1024 entries, each describing a page table or marking a region as not present. The CR3 register points to the current page directory.

**Page Tables**: Each page directory entry points to a page table containing 1024 entries. Each entry maps one 4KB virtual page to a physical frame.

This two-level structure allows sparse virtual address spaces to be represented efficiently - if a page table isn't needed (all pages not present), it doesn't have to exist in memory.

### Address Translation

Virtual to physical translation works as follows:

1. Extract bits 22-31 of the virtual address (page directory index)
2. Use this to index into the page directory
3. If the present bit is clear, the address is not mapped
4. Extract bits 12-21 of the virtual address (page table index)
5. Use this to index into the page table
6. If the present bit is clear, the address is not mapped
7. Extract the physical frame address from the page table entry
8. Add bits 0-11 of the virtual address (page offset) to get final physical address

The hardware performs this translation automatically on every memory access when paging is enabled.

### Page Mapping

To map a virtual page to a physical frame:

1. Calculate page directory and page table indices from the virtual address
2. Check if a page table exists for this directory entry
3. If not, allocate a frame and create a new page table
4. Set the page directory entry to point to the page table
5. Set the page table entry to point to the physical frame
6. Set appropriate flags (present, writable, user-accessible, etc.)

### Page Unmapping

Unmapping a page:

1. Calculate page directory and page table indices
2. Clear the page table entry
3. Invalidate the Translation Lookaside Buffer (TLB) for this address

The TLB caches recent address translations. When a mapping changes, the corresponding TLB entry must be invalidated or stale translations may be used.

### Identity Mapping

The kernel is identity-mapped, meaning its virtual addresses equal physical addresses. This simplifies kernel programming since physical memory can be accessed directly without complex address calculations.

The first 8MB of virtual memory is identity-mapped to physical memory, covering the kernel code and data. This mapping exists in all page directories so kernel code remains accessible regardless of which process is active.

### Initialization

VMM initialization:

1. Allocates a page directory for the kernel
2. Creates identity mappings for the first 8MB
3. Maps the VGA text buffer at 0xB8000
4. Enables paging by setting the CR0 register
5. Records the kernel page directory for future use

### Directory Switching

When switching between processes, the VMM loads the new process's page directory into CR3. This single register write instantly changes the entire virtual address space, providing strong isolation between processes.

## Heap Allocator

### Design Goals

The heap allocator provides malloc/free style dynamic memory allocation for kernel code. Key requirements include:

- Variable-sized allocations
- Efficient space utilization
- Reasonable performance
- Simple implementation

### Block Structure

The heap is organized as a linked list of blocks. Each block has a header containing:

- Size of the data area (not including header)
- Free flag (1 if available, 0 if allocated)
- Pointer to the next block

Data immediately follows the header. Users receive a pointer to the data area, keeping the header hidden.

### Allocation Strategy

The allocator uses a first-fit strategy:

1. Walk the list of blocks
2. Find the first free block large enough for the request
3. If the block is significantly larger, split it into two blocks
4. Mark the block as allocated
5. Return a pointer to the data area

If no suitable block exists, the heap is expanded by mapping additional virtual pages backed by physical frames.

### Splitting

When allocating from a free block much larger than needed:

1. Calculate space needed (requested size + header)
2. If remaining space exceeds a threshold, create a new block
3. New block starts after the allocated data
4. New block header is initialized with remaining size
5. Original block's size is reduced to requested size
6. Link the new block into the list

This prevents internal fragmentation by avoiding waste of large blocks on small allocations.

### Deallocation

Freeing memory marks the block as available but doesn't immediately return it to the system:

1. Find the block header (back up by sizeof(header) from the pointer)
2. Mark the block as free
3. Attempt to coalesce with adjacent free blocks

### Coalescing

After freeing a block, the allocator walks the list looking for adjacent free blocks:

1. If the current block is free and next block is free
2. Combine them by extending the current block's size
3. Skip over the next block's header
4. Update the link to point past the merged block

Coalescing prevents external fragmentation by reuniting split blocks into larger usable chunks.

### Heap Expansion

When allocation fails to find a suitable block:

1. Calculate how much space is needed
2. Round up to page boundaries
3. Allocate physical frames via PMM
4. Map them into the heap virtual address range via VMM
5. Create a new free block spanning the new memory
6. Link it into the block list

The heap grows incrementally from an initial size (1MB) up to a maximum limit (256MB).

### Performance Characteristics

First-fit allocation is O(n) in the number of blocks, but performs well when most allocations are quickly satisfied. Deallocation is also O(n) due to coalescing.

More sophisticated algorithms like segregated free lists or buddy allocation could improve performance but add complexity. The current implementation balances simplicity and efficiency for kernel use.

## Integration

These three layers work together seamlessly:

1. VMM uses PMM to allocate frames for page tables
2. Heap uses VMM to map new virtual memory ranges
3. Heap uses PMM to obtain physical backing for virtual pages

This separation of concerns keeps each component focused on a single responsibility while providing powerful combined functionality.

## Memory Safety

Several mechanisms protect against memory errors:

- Paging enforces access permissions (writable vs read-only)
- NULL pointer dereferences trigger page faults
- Heap headers include sanity checks
- Double-free detection prevents corruption
- All allocations are validated before use

While not foolproof, these checks catch most common programming errors and prevent corruption from propagating through the system.

## Debugging Support

The memory subsystem includes instrumentation for debugging:

- Counters track total allocations and frees
- Memory usage can be queried at any time
- Page faults report the faulting address
- Serial port logging captures allocation failures

This diagnostic information is invaluable when tracking down memory leaks or corruption bugs.

---

**Implementation Files**:
- `kernel/memory/pmm.c` - Physical memory manager
- `kernel/memory/vmm.c` - Virtual memory manager  
- `kernel/memory/heap.c` - Heap allocator
- `kernel/memory/utils.c` - Memory manipulation utilities
- `kernel/memory/memory.h` - Public API definitions
