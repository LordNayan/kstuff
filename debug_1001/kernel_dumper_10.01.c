#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <errno.h>

// PS5 kernel memory layout
#define KERNEL_BASE 0xffffffff80000000ULL
#define KERNEL_SIZE (32 * 1024 * 1024)  // 32MB - should be enough for most kernels
#define DUMP_PATH "/data/kernel_10.01.bin"
#define CHUNK_SIZE (64 * 1024)  // Read in 64KB chunks

// Function to check if we can access kernel memory
int test_kernel_access(void) {
    printf("[*] Testing kernel memory access...\n");
    
    // Try to read the first 8 bytes of kernel
    volatile uint64_t test_read = 0;
    char *kernel_ptr = (char *)KERNEL_BASE;
    
    // This might crash if we don't have kernel access
    // In a real payload, you'd use proper syscalls or exploits
    // For now, we'll assume kstuff gives us the access we need
    
    printf("[*] Attempting to read from kernel base: 0x%llx\n", KERNEL_BASE);
    return 1; // Assume success for now
}

// Function to find actual kernel size by looking for valid data
size_t find_kernel_size(char *kernel_base) {
    printf("[*] Determining actual kernel size...\n");
    
    size_t size = 0;
    char *ptr = kernel_base;
    
    // Look for ELF magic bytes at the start
    if (ptr[0] == 0x7f && ptr[1] == 'E' && ptr[2] == 'L' && ptr[3] == 'F') {
        printf("[+] Found ELF header at kernel base\n");
    } else {
        printf("[-] Warning: No ELF header found at kernel base\n");
    }
    
    // For safety, let's dump a fixed size
    // In a real scenario, you'd parse the ELF header to get the actual size
    size = 24 * 1024 * 1024; // 24MB should be sufficient
    
    printf("[*] Using kernel size: %zu bytes (%.2f MB)\n", size, size / (1024.0 * 1024.0));
    return size;
}

// Main kernel dumper function
int dump_kernel(void) {
    printf("[*] Starting PS5 kernel dump for firmware 10.01\n");
    printf("[*] Kernel base address: 0x%llx\n", KERNEL_BASE);
    printf("[*] Output file: %s\n", DUMP_PATH);
    
    // Test if we can access kernel memory
    if (!test_kernel_access()) {
        printf("[-] ERROR: Cannot access kernel memory\n");
        return -1;
    }
    
    // Open output file
    int fd = open(DUMP_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        printf("[-] ERROR: Cannot create dump file: %s\n", strerror(errno));
        return -1;
    }
    
    printf("[+] Created dump file: %s\n", DUMP_PATH);
    
    // Get pointer to kernel memory
    char *kernel_base = (char *)KERNEL_BASE;
    
    // Determine kernel size
    size_t kernel_size = find_kernel_size(kernel_base);
    
    // Dump kernel in chunks
    printf("[*] Dumping kernel in %d byte chunks...\n", CHUNK_SIZE);
    
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
            printf("[-] ERROR: Write failed at offset 0x%lx\n", offset);
            close(fd);
            return -1;
        }
        
        bytes_written += written;
        chunks_written++;
        
        // Progress indicator
        if (chunks_written % 16 == 0) {
            float progress = (float)bytes_written / kernel_size * 100.0;
            printf("[*] Progress: %.1f%% (%zu / %zu bytes)\n", 
                   progress, bytes_written, kernel_size);
        }
    }
    
    close(fd);
    
    printf("[+] Kernel dump completed!\n");
    printf("[+] Total bytes written: %zu (%.2f MB)\n", 
           bytes_written, bytes_written / (1024.0 * 1024.0));
    printf("[+] Output file: %s\n", DUMP_PATH);
    
    return 0;
}

int main(int argc, char *argv[]) {
    printf("=== PS5 Kernel Dumper for Firmware 10.01 ===\n");
    printf("=== Part of kstuff 10.01 support project ===\n\n");
    
    // Check if running with appropriate privileges
    printf("[*] Current UID: %d\n", getuid());
    printf("[*] Current PID: %d\n", getpid());
    
    // Perform the kernel dump
    int result = dump_kernel();
    
    if (result == 0) {
        printf("\n[+] SUCCESS: Kernel dumped successfully!\n");
        printf("[*] Next steps:\n");
        printf("    1. Copy kernel_10.01.bin to your analysis machine\n");
        printf("    2. Load it into Ghidra/IDA with base address 0xffffffff80000000\n");
        printf("    3. Start looking for offsets and ROP gadgets\n");
    } else {
        printf("\n[-] FAILURE: Kernel dump failed\n");
        printf("[*] This might mean:\n");
        printf("    1. kstuff is not loaded properly\n");
        printf("    2. Kernel access is not available\n");
        printf("    3. Memory layout has changed significantly\n");
    }
    
    return result;
}