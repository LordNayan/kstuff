/*
 * Instrumented kstuff for 10.01 Offset Discovery
 * 
 * This is a modified version of ps5-kstuff/main.c with extensive logging
 * to help discover correct offsets and parasite points.
 * 
 * Build instructions:
 * 1. Copy this to ps5-kstuff/main_debug.c
 * 2. Modify Makefile to build debug version
 * 3. Deploy to PS5 and check /data/kstuff_discovery.log
 */

// Add to top of ps5-kstuff/main.c or create wrapper

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

static int debug_log_fd = -1;

static void init_debug_log(void)
{
    debug_log_fd = open("/data/kstuff_discovery.log", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(debug_log_fd >= 0)
    {
        const char* header = "=== kstuff 10.01 Discovery Log ===\n";
        write(debug_log_fd, header, strlen(header));
    }
}

static void debug_log(const char* msg)
{
    if(debug_log_fd < 0) return;
    write(debug_log_fd, msg, strlen(msg));
    write(debug_log_fd, "\n", 1);
}

static void debug_log_hex(const char* msg, uint64_t value)
{
    if(debug_log_fd < 0) return;
    
    char buf[256];
    char* p = buf;
    
    // Copy message
    while(*msg)
        *p++ = *msg++;
    
    *p++ = ' ';
    *p++ = '=';
    *p++ = ' ';
    *p++ = '0';
    *p++ = 'x';
    
    // Convert hex
    for(int i = 15; i >= 0; i--)
    {
        int nibble = (value >> (i * 4)) & 0xF;
        *p++ = nibble < 10 ? '0' + nibble : 'A' + nibble - 10;
    }
    
    *p++ = '\n';
    
    write(debug_log_fd, buf, p - buf);
}

// Hook this into the main() function
static void discover_parasites_mode(void)
{
    init_debug_log();
    
    debug_log("=== PARASITE DISCOVERY MODE ===");
    debug_log_hex("Kernel base (kdata_base)", kdata_base);
    
    // Test basic memory access
    debug_log("Testing kernel memory access...");
    
    uint64_t test_read;
    if(copyout(&test_read, kdata_base, 8) == 0)
    {
        debug_log("✓ Kernel read works");
        debug_log_hex("First 8 bytes", test_read);
    }
    else
    {
        debug_log("✗ Kernel read FAILED - check r0gdb initialization");
        return;
    }
    
    // Test IDT access
    debug_log_hex("IDT address", offsets.idt);
    
    uint64_t idt_entry;
    if(copyout(&idt_entry, offsets.idt, 8) == 0)
    {
        debug_log("✓ IDT read works");
        debug_log_hex("IDT first entry", idt_entry);
    }
    else
    {
        debug_log("✗ IDT read FAILED - check idt offset");
    }
    
    // Test allproc
    debug_log_hex("allproc address", offsets.allproc);
    
    uint64_t allproc_ptr;
    if(copyout(&allproc_ptr, offsets.allproc, 8) == 0)
    {
        debug_log("✓ allproc read works");
        debug_log_hex("allproc value", allproc_ptr);
    }
    else
    {
        debug_log("✗ allproc read FAILED - check allproc offset");
    }
    
    // Test ROP gadgets (just verify addresses are reasonable)
    debug_log("=== ROP GADGET ADDRESSES ===");
    debug_log_hex("doreti_iret", (uint64_t)offsets.doreti_iret + kdata_base);
    debug_log_hex("push_pop_all_iret", (uint64_t)offsets.push_pop_all_iret + kdata_base);
    debug_log_hex("wrmsr_ret", (uint64_t)offsets.wrmsr_ret + kdata_base);
    debug_log_hex("rdmsr_start", (uint64_t)offsets.rdmsr_start + kdata_base);
    
    // Log parasite points
    debug_log("=== PARASITE POINTS (UNVERIFIED) ===");
    struct parasite_desc* desc = get_parasites(&desc_size);
    if(desc)
    {
        debug_log("Syscall parasites:");
        for(int i = 0; i < desc->lim_syscall; i++)
        {
            debug_log_hex("  parasite", desc->parasites[i].address);
        }
        
        debug_log("FSELF parasites:");
        for(int i = desc->lim_syscall; i < desc->lim_fself; i++)
        {
            debug_log_hex("  parasite", desc->parasites[i].address);
        }
        
        debug_log("Unsorted parasites:");
        for(int i = desc->lim_fself; i < desc->lim_total; i++)
        {
            debug_log_hex("  parasite", desc->parasites[i].address);
        }
    }
    
    debug_log("=== DISCOVERY LOG COMPLETE ===");
    debug_log("Next steps:");
    debug_log("1. If kernel read works, offsets are partially correct");
    debug_log("2. Try loading a simple homebrew ELF");
    debug_log("3. Check uelf logs for parasite encounters");
    debug_log("4. Validate each parasite individually");
    
    if(debug_log_fd >= 0)
        close(debug_log_fd);
}

// Add to uelf/main.c in the handle() function to log parasite encounters:

static void log_parasite_encounter(uint64_t rip, int reg_idx, const char* category)
{
    int fd = open("/data/kstuff_parasites.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if(fd < 0) return;
    
    char buf[256];
    char* p = buf;
    
    // Category
    while(*category)
        *p++ = *category++;
    *p++ = ':';
    *p++ = ' ';
    
    // RIP
    *p++ = '0';
    *p++ = 'x';
    for(int i = 15; i >= 0; i--)
    {
        int nibble = (rip >> (i * 4)) & 0xF;
        *p++ = nibble < 10 ? '0' + nibble : 'A' + nibble - 10;
    }
    
    *p++ = ' ';
    *p++ = 'R';
    *p++ = '0' + reg_idx;
    *p++ = '\n';
    
    write(fd, buf, p - buf);
    close(fd);
}

// Modify uelf/main.c handle_parasites() to log:
static inline int handle_parasites_logged(uint64_t* regs, int a, int b, const char* category)
{
    for(int i = a; i < b; i++)
        if(parasites.parasites[i].address == regs[RIP])
        {
            // LOG THIS!
            log_parasite_encounter(regs[RIP], parasites.parasites[i].reg, category);
            
            for(int j = i; j < b && parasites.parasites[j].address == regs[RIP]; j++)
                regs[parasites.parasites[j].reg] |= -1ull << 48;
            return 1;
        }
    return 0;
}

// Also add logging for unknown parasite-like situations:
// In uelf/main.c around line 200, modify DECRYPT macro:

#define DECRYPT_LOG(which, idx) \
    if((regs[which] >> 48) == 0xdeb7) { \
        /* This is an UNDISCOVERED parasite point! */ \
        log_parasite_encounter(regs[RIP], idx, "DISCOVERED"); \
        regs[which] |= 0xffffull << 48; \
        decrypted = 1; \
    }

/*
 * USAGE INSTRUCTIONS:
 * 
 * 1. Add discover_parasites_mode() call at the start of main() in ps5-kstuff/main.c:
 * 
 *    int main(void* ds, int a, int b, uintptr_t c, uintptr_t d)
 *    {
 *        if(r0gdb_init(ds, a, b, c, d)) {
 *            notify("your firmware is not supported (prosper0gdb)");
 *            return 1;
 *        }
 *        
 *        // Add this:
 *        #ifdef DISCOVER_MODE
 *        discover_parasites_mode();
 *        #endif
 *        
 *        // ... rest of main
 *    }
 * 
 * 2. Build with: make EXTRA_CFLAGS="-DDISCOVER_MODE"
 * 
 * 3. Deploy to PS5 and run
 * 
 * 4. Check these files:
 *    /data/kstuff_discovery.log - Initial offset validation
 *    /data/kstuff_parasites.log - Parasite encounters during operation
 * 
 * 5. For real parasite discovery, do operations that trigger them:
 *    - Run syscalls: open("/system/common/lib/libc.sprx", ...)
 *    - Load ELFs: execve() on homebrew
 *    - Mount filesystem: mount operations
 *    - Install PKG: trigger installer
 * 
 * 6. Collect all "DISCOVERED" entries from kstuff_parasites.log
 * 
 * 7. Those are your real 10.01 parasite addresses!
 */
