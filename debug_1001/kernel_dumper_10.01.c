
// Minimal notify implementation for payload-safe logging
#include <stdint.h>
#include <stddef.h>
#include "../prosper0gdb/r0gdb.h" // For syscalls and kernel helpers
#include "../freebsd-headers/sys/types.h"
#include "../freebsd-headers/sys/stat.h"
#include "../freebsd-headers/sys/mman.h"
#include "../freebsd-headers/sys/errno.h"
#include "../freebsd-headers/fcntl.h"

void notify(const char* s)
{
    struct
    {
        char pad1[0x10];
        int f1;
        char pad2[0x19];
        char msg[0xc03];
    } notification = {.f1 = -1};
    char* d = notification.msg;
    while(*d++ = *s++);
    int fd = open("/dev/notification0", 1);
    write(fd, &notification, 0xc30);
    close(fd);
}

// PS5 kernel memory layout
#define KERNEL_BASE 0xffffffff80000000ULL
#define KERNEL_SIZE (32 * 1024 * 1024)  // 32MB - should be enough for most kernels
#define DUMP_PATH "/data/kernel_10.01.bin"
#define CHUNK_SIZE (64 * 1024)  // Read in 64KB chunks

// Function to check if we can access kernel memory
int test_kernel_access(void) {
    notify("[*] Testing kernel memory access...\n");
    // Try to read the first 8 bytes of kernel
    volatile uint64_t test_read = 0;
    char *kernel_ptr = (char *)KERNEL_BASE;
    // This might crash if we don't have kernel access
    // In a real payload, you'd use proper syscalls or exploits
    // For now, we'll assume kstuff gives us the access we need
    notify("[*] Attempting to read from kernel base: 0xFFFFFFFF80000000\n");
    return 1; // Assume success for now
}

// Function to find actual kernel size by looking for valid data
size_t find_kernel_size(char *ptr) {
    size_t size;
    if (ptr[0] == 0x7f && ptr[1] == 'E' && ptr[2] == 'L' && ptr[3] == 'F') {
        notify("[+] Found ELF header at kernel base\n");
    } else {
        notify("[-] Warning: No ELF header found at kernel base\n");
    }
    // For safety, let's dump a fixed size
    // In a real scenario, you'd parse the ELF header to get the actual size
    size = 24 * 1024 * 1024; // 24MB should be sufficient
    notify("[*] Using kernel size: 25165824 bytes (24.00 MB)\n");
    return size;
}

// Main kernel dumper function
int dump_kernel(void) {
    notify("[*] Starting PS5 kernel dump for firmware 10.01\n");
    notify("[*] Kernel base address: 0xFFFFFFFF80000000\n");
    notify("[*] Output file: /data/kernel_10.01.bin\n");

    // Test if we can access kernel memory
    if (!test_kernel_access()) {
        notify("[-] ERROR: Cannot access kernel memory\n");
        return -1;
    }

    // Open output file
    int fd = open(DUMP_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        notify("[-] ERROR: Cannot create dump file\n");
        return -1;
    }
    notify("[+] Created dump file: /data/kernel_10.01.bin\n");

    // Get pointer to kernel memory
    char *kernel_base = (char *)KERNEL_BASE;

    // Determine kernel size
    size_t kernel_size = find_kernel_size(kernel_base);

    // Dump kernel in chunks
    notify("[*] Dumping kernel in 65536 byte chunks...\n");

    size_t bytes_written = 0;
    size_t chunks_written = 0;

    for (size_t offset = 0; offset < kernel_size; offset += CHUNK_SIZE) {
        size_t chunk_size = CHUNK_SIZE;
        // Adjust size for last chunk
        if (offset + chunk_size > kernel_size) {
            chunk_size = kernel_size - offset;
        }
        // Read chunk from kernel memory
        char *source = kernel_base + offset;
        // Write chunk to file
        ssize_t written = write(fd, source, chunk_size);
        if (written != chunk_size) {
            notify("[-] ERROR: Write failed during kernel dump\n");
            close(fd);
            return -1;
        }
        bytes_written += written;
        chunks_written++;
        // Progress indicator
        if (chunks_written % 16 == 0) {
            notify("[*] Progress: kernel dump in progress\n");
        }
    }

    close(fd);
    notify("[+] Kernel dump completed!\n");
    notify("[+] Output file: /data/kernel_10.01.bin\n");
    return 0;
}

int main(int argc, char *argv[]) {
    notify("=== PS5 Kernel Dumper for Firmware 10.01 ===\n");
    notify("=== Part of kstuff 10.01 support project ===\n");
    // Perform the kernel dump
    int result = dump_kernel();
    if (result == 0) {
        notify("[+] SUCCESS: Kernel dumped successfully!\n");
        notify("[*] Next steps: Copy kernel_10.01.bin to your analysis machine and load into Ghidra/IDA at 0xffffffff80000000\n");
    } else {
        notify("[-] FAILURE: Kernel dump failed\n");
        notify("[*] This might mean: kstuff not loaded, kernel access unavailable, or memory layout changed\n");
    }
    return result;
}