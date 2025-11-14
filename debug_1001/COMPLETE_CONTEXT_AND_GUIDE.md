# PS5 10.01 kstuff Support - Complete Context & Instructions

## Current Situation Summary

### What Works
- ✅ **PS5 Firmware:** 10.01
- ✅ **Jailbreak Method:** Y2JB (userland exploit)
- ✅ **ELF Loader:** Enabled on port 9021 after Y2JB
- ✅ **Payload Injection:** Can send payloads from PC to PS5
- ✅ **kstuff Status:** Works perfectly up to firmware 9.60
- ❌ **kstuff on 10.0x:** Does NOT work (neither 10.00 nor 10.01)

### Repository Information
- **Repo:** EchoStretch/kstuff
- **Branch:** elfldr-compatability
- **Location:** `/Users/nayan/kstuff`
- **Purpose:** PS5 kernel exploitation toolkit enabling homebrew

### What kstuff Does
kstuff is a kernel-level exploitation payload that:
1. Provides kernel read/write primitives
2. Enables loading of unsigned ELF files (homebrew)
3. Hooks syscalls for extended functionality
4. Patches system files (SceShellCore) for features
5. Implements GDB debugging stub for kernel
6. Handles SELF file decryption
7. Enables PKG installation
8. Provides FTP server and other utilities

---

## Why 10.01 Doesn't Work - Root Cause Analysis

### Problem Identified
In commit `a1932bb` (Added 10.0x Offsets - Not Working):
- Offsets were added for 10.00 and 10.01
- **BUT they don't work**

### Technical Analysis Completed

#### 1. **Parasite Addresses Are Wrong** (CRITICAL ISSUE)
   
**What are parasites?**
- Specific kernel code addresses where kstuff injects hooks
- Used to decrypt encrypted pointers (sign-extension: `|= -1ull << 48`)
- Intercept SELF file loading, syscalls, and DRM operations
- Total needed: 14 addresses (3 syscall + 9 FSELF + 2 unsorted)

**Current problem:**
```python
# From validation script output:
❌ Too many parasites (6) match 10.00 - likely not discovered properly!
⚠️ Duplicate parasite addresses found
```

**Why this happens:**
- Parasites were copied from 10.00 to 10.01
- Firmware updates change code layout
- Wrong addresses = crashes or silent failures
- These MUST be discovered via runtime analysis

**Current (wrong) parasites in code:**
```c
// In ps5-kstuff/main.c
static struct PARASITES(14) parasites_1001 = {
    .lim_syscall = 3,
    .lim_fself = 12,
    .lim_total = 14,
    .parasites = {
        /* syscall parasites */
        {-0x894308, R13},  // ← COPIED FROM 10.00, LIKELY WRONG
        {-0x3BF480, RSI},  // ← COPIED FROM 10.00, LIKELY WRONG
        {-0x3BF440, RSI},  // ← COPIED FROM 10.00, LIKELY WRONG
        // ... etc - all likely wrong
    }
};
```

#### 2. **ROP Gadget Mismatch** (IMPORTANT)

**Critical gadget difference:**
| Gadget | 10.00 | 10.01 | Issue |
|--------|-------|-------|-------|
| `push_pop_all_iret` | `-0xa106b8` | `-0xa10540` | Different! Must verify |

- This gadget is used in `kelf.asm` for register manipulation
- Wrong address = immediate crash
- Difference of `0x178` bytes between versions suggests one is wrong

#### 3. **Most Other Offsets Look Reasonable**
- Data structures (allproc, idt, pcpu_array) - probably correct
- Syscall infrastructure (sysents, syscall_before) - needs verification
- Crypto functions (sceSblServiceMailbox) - needs verification

---

## What Needs To Be Done

### Phase 1: Static Analysis (Find Basic Offsets)
**Tools needed:** Ghidra or IDA Pro

**Tasks:**
1. **Dump PS5 10.01 kernel binary**
   - Use Y2JB + custom payload to dump kernel memory
   - Starting address: `0xffffffff80000000`
   - Save to file for analysis

2. **Load into Ghidra**
   - Architecture: x86-64
   - Base address: 0xffffffff80000000
   - Run auto-analysis

3. **Find/verify basic offsets:**
   - `allproc` - Process list head
   - `idt` - Interrupt Descriptor Table
   - `sysents` - Syscall table
   - `pcpu_array` - Per-CPU data
   - `malloc` - Kernel malloc function

4. **Find ROP gadgets:**
   - `doreti_iret` - Pattern: `48 CF` (iretq)
   - `push_pop_all_iret` - Pattern: `50 51 52 53...48 CF` (push all regs + iretq)
   - `wrmsr_ret` - Pattern: `0F 30 C3` (wrmsr; ret)
   - `rdmsr_start` - Pattern: `0F 32` (rdmsr)
   - `rep_movsb_pop_rbp_ret` - Pattern: `F3 A4 5D C3`

### Phase 2: Dynamic Analysis (Find Parasites) - MOST CRITICAL
**This is the main work and why 10.01 doesn't work!**

**Method:**
1. **Instrument kstuff payload** with logging
2. **Deploy to PS5** via Y2JB + port 9021
3. **Trigger operations** that hit parasite points
4. **Collect addresses** from logs
5. **Update code** with real addresses

**Operations to trigger:**
- Open system SELF files
- Load homebrew ELF
- Execute programs
- Mount filesystems
- Install PKG files
- Socket operations

**Expected logging output:**
```
DISCOVERED: 0xffffffff8076bcf8 R13   ← syscall parasite 1
DISCOVERED: 0xffffffff82c40b80 RSI   ← syscall parasite 2
DISCOVERED: 0xffffffff82d039ba RAX   ← FSELF parasite 1
... etc
```

### Phase 3: Integration & Testing
1. Update `prosper0gdb/offsets.c` with verified offsets
2. Update `ps5-kstuff/main.c` with discovered parasites
3. Rebuild payload
4. Test incrementally (one component at a time)
5. Validate stability

---

## Files Created for This Task

Located in `/Users/nayan/kstuff/`:

### 1. `ADDING_10.01_SUPPORT.md`
- **Purpose:** Comprehensive technical guide
- **Contents:** 
  - Detailed explanation of every component
  - Step-by-step discovery methods
  - Code patterns to search for
  - Debugging techniques
- **Use when:** Learning how everything works

### 2. `QUICK_REFERENCE.md`
- **Purpose:** Checklist-style quick reference
- **Contents:**
  - Task checklists with checkboxes
  - Quick lookup tables
  - Common mistakes to avoid
  - Pro tips
- **Use when:** Actually doing the work

### 3. `debug_instrumentation.c`
- **Purpose:** Logging code for parasite discovery
- **Contents:**
  - Functions to add to main.c
  - Logging infrastructure
  - Parasite detection code
  - Usage instructions
- **Use when:** Building instrumented payload

### 4. `discover_offsets.py`
- **Purpose:** Python validation and helper tool
- **Contents:**
  - Compare 9.60 vs 10.01 offsets
  - Validate for common issues
  - Generate C code
  - Interactive offset entry
- **Usage:**
  ```bash
  python3 discover_offsets.py --mode compare    # Show differences
  python3 discover_offsets.py --mode validate   # Check for errors
  python3 discover_offsets.py --mode generate   # Generate C code
  python3 discover_offsets.py --mode interactive # Enter offsets
  ```

### 5. `offset_finder_10.01.c`
- **Purpose:** Template payload for offset discovery
- **Contents:** Basic structure for memory scanning payload

---

## Technical Deep Dive

### Understanding the kstuff Architecture

```
┌─────────────────────────────────────────┐
│  User sends payload via Y2JB (port 9021)│
└───────────────┬─────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────┐
│  ps5-kstuff-ldr/main.c                  │
│  - Loads embedded payload               │
│  - Maps to memory                       │
│  - Calls entry point                    │
└───────────────┬─────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────┐
│  ps5-kstuff/main.c                      │
│  - Initializes r0gdb (kernel access)    │
│  - Loads offsets for firmware version   │
│  - Allocates kernel memory              │
│  - Injects kelf (kernel exploit code)   │
│  - Injects uelf (userland ELF loader)   │
│  - Patches SceShellCore                 │
└───────────────┬─────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────┐
│  prosper0gdb/                           │
│  - Provides kernel read/write           │
│  - GDB debugging stub                   │
│  - Kernel function calling              │
│  - Uses firmware-specific offsets       │
└─────────────────────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────┐
│  ps5-kstuff/kelf.asm                    │
│  - Kernel ROP chain execution           │
│  - Hooks IDT interrupts                 │
│  - Switches to uelf context             │
└─────────────────────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────┐
│  ps5-kstuff/uelf/                       │
│  - Handles syscall interception         │
│  - Processes parasite hooks             │
│  - Decrypts SELF files                  │
│  - Loads homebrew ELF                   │
│  - Manages kekcalls                     │
└─────────────────────────────────────────┘
```

### Critical Code Locations

#### Offset Definitions
```c
// File: prosper0gdb/offsets.c
// Lines: 2882-3070 (for 10.00 and 10.01)

START_FW(1001)
DEF(allproc, 0x2765d70)      // Process list
DEF(idt, 0x2d5c300)          // Interrupt table
DEF(push_pop_all_iret, -0xa10540)  // ROP gadget ← VERIFY THIS
// ... 90+ more offsets
#include "offset_list.txt"
END_FW()
```

#### Parasite Definitions
```c
// File: ps5-kstuff/main.c
// Lines: 2380-2408 (for 10.01)

static struct PARASITES(14) parasites_1001 = {
    .lim_syscall = 3,   // First 3 are syscall parasites
    .lim_fself = 12,    // Next 9 are FSELF parasites
    .lim_total = 14,    // Last 2 are unsorted
    .parasites = {
        {-0x894308, R13},  // ← ALL THESE NEED DISCOVERY
        // ... 13 more
    }
};
```

#### Parasite Handler
```c
// File: ps5-kstuff/uelf/main.c
// Lines: 135, 173, 175

// Called from handle() function
if(handle_syscall_parasites(regs))   // Check syscall parasites
    return;
// ...
else if(handle_fself_parasites(regs)) // Check FSELF parasites
    return;
else if(handle_unsorted_parasites(regs)) // Check other parasites
    return;
```

#### Parasite Detection Logic
```c
// File: ps5-kstuff/uelf/parasites.h
// Lines: 7-18

static inline int handle_parasites(uint64_t* regs, int a, int b)
{
    for(int i = a; i < b; i++)
        if(parasites.parasites[i].address == regs[RIP])  // Check if RIP matches
        {
            for(int j = i; j < b && parasites.parasites[j].address == regs[RIP]; j++)
                regs[parasites.parasites[j].reg] |= -1ull << 48;  // Decrypt pointer
            return 1;
        }
    return 0;
}
```

---

## Practical Workflow

### Step-by-Step Process

#### Week 1: Preparation & Static Analysis

**Day 1-2: Setup**
```bash
# 1. Review documentation
cd /Users/nayan/kstuff
cat ADDING_10.01_SUPPORT.md      # Full guide
cat QUICK_REFERENCE.md            # Checklist

# 2. Install tools
# - Ghidra (https://ghidra-sre.org/)
# - Python 3
# - Hex editor

# 3. Dump kernel
# Create payload to dump /boot/kernel/kernel or kernel memory
# Deploy via Y2JB to port 9021
```

**Day 3-4: Find Basic Offsets**
```
1. Load kernel dump in Ghidra
2. Search for strings: "allproc", "malloc", etc.
3. Find ROP gadgets using instruction search
4. Document all findings
```

#### Week 2: Dynamic Discovery (CRITICAL)

**Day 5-7: Instrument Payload**
```bash
# 1. Add logging from debug_instrumentation.c
cd /Users/nayan/kstuff/ps5-kstuff

# 2. Modify main.c to add:
# - init_debug_log()
# - debug_log_hex()
# - discover_parasites_mode()

# 3. Modify uelf/main.c to add:
# - log_parasite_encounter()
# - DECRYPT_LOG macro

# 4. Build with discovery mode
make EXTRA_CFLAGS="-DDISCOVER_MODE"
```

**Day 8-10: Discovery Testing**
```
1. Deploy instrumented payload to PS5 (Y2JB → port 9021)

2. Trigger syscall parasites:
   - open("/system/common/lib/libc.sprx")
   - socket operations
   - Various syscalls

3. Trigger FSELF parasites:
   - Load homebrew ELF
   - Load system SELF files
   - Execute programs

4. Trigger unsorted parasites:
   - Mount filesystems
   - Crypto operations
   - PKG installation

5. Collect from /data/kstuff_parasites.log:
   DISCOVERED: 0xffffffff8XXXXXXX R13
   DISCOVERED: 0xffffffff8XXXXXXX RSI
   ... etc (should get 14 total)
```

**Day 11-12: Verify & Validate**
```bash
# 1. Update discover_offsets.py with found addresses
# Edit PARASITES_1001 dictionary

# 2. Validate
python3 discover_offsets.py --mode validate

# Expected output: ✅ All validation checks passed!
```

#### Week 3: Integration & Testing

**Day 13-14: Code Integration**
```c
// 1. Update prosper0gdb/offsets.c
START_FW(1001)
// Add all verified offsets
END_FW()

// 2. Update ps5-kstuff/main.c
static struct PARASITES(14) parasites_1001 = {
    // Add all discovered parasite addresses
};

// 3. Rebuild
make clean
make
```

**Day 15-16: Testing**
```
Test progression:
1. Payload loads without crash ✓
2. Can read kernel memory ✓
3. Syscall interception works ✓
4. Can load homebrew ELF ✓
5. Can load signed SELF ✓
6. System stable for 1+ hour ✓
```

**Day 17: Documentation & PR**
```bash
# Document findings
# Create pull request
# Share with community
```

---

## How to Use Y2JB + kstuff Together

### Current Workflow
```
1. PS5 boots (firmware 10.01)
2. Run Y2JB jailbreak exploit
3. Debug settings appear
4. ELF loader enabled on port 9021
5. From PC: Send kstuff payload to PS5:9021
6. ❌ Currently crashes (wrong offsets/parasites)
7. ✅ After fix: kstuff loads, homebrew enabled
```

### After Fixing 10.01 Support
```bash
# On PC:
cd /Users/nayan/kstuff/ps5-kstuff-ldr
make

# Send to PS5
nc <PS5_IP> 9021 < kstuff.elf

# Or use existing deployment:
make test PS5_HOST=<PS5_IP> PS5_PORT=9021
```

---

## Known Issues & Solutions

### Issue: Parasites Identical to 10.00
**Symptom:** 6+ parasites match 10.00 exactly  
**Cause:** Copied instead of discovered  
**Solution:** Must discover via runtime analysis (Phase 2)

### Issue: push_pop_all_iret Different Between 10.00/10.01
**Symptom:** Different offset values  
**Cause:** Code moved between versions  
**Solution:** Verify in Ghidra for 10.01 kernel

### Issue: Payload Crashes Immediately
**Symptom:** Crash on load  
**Cause:** Bad ROP gadget offsets  
**Solution:** Verify doreti_iret, push_pop_all_iret first

### Issue: Payload Hangs
**Symptom:** Freeze during load  
**Cause:** Bad wrmsr_ret or infinite loop  
**Solution:** Check all iret-based gadgets

### Issue: Works Briefly Then Crashes
**Symptom:** Loads OK, crashes on first operation  
**Cause:** Bad parasite offsets  
**Solution:** Disable all parasites, enable one-by-one

---

## Key Concepts to Understand

### 1. Parasites
- **What:** Kernel code addresses for pointer decryption hooks
- **Why:** PS5 encrypts kernel pointers (top 16 bits = 0xdeb7)
- **How:** kstuff intercepts these locations and decrypts (`|= -1ull << 48`)
- **Count:** 14 total (3 syscall + 9 FSELF + 2 unsorted)

### 2. ROP Gadgets
- **What:** Code sequences for return-oriented programming
- **Why:** Needed to execute code in kernel context
- **How:** Chain together existing code snippets
- **Critical:** doreti_iret, push_pop_all_iret, wrmsr_ret

### 3. Firmware Offsets
- **What:** Memory addresses of kernel structures/functions
- **Why:** Access kernel internals
- **How:** Static analysis + runtime validation
- **Count:** 90+ offsets per firmware

### 4. SELF Files
- **What:** Signed Encrypted ELF format
- **Why:** PS5's code signing mechanism
- **How:** kstuff hooks decryption process
- **Impact:** Enables homebrew loading

---

## Validation Checklist

Before deploying to PS5:
- [ ] All 14 parasites discovered (not copied)
- [ ] No duplicate parasite addresses
- [ ] push_pop_all_iret verified in Ghidra
- [ ] All ROP gadgets found and validated
- [ ] python3 discover_offsets.py --mode validate passes
- [ ] Code builds without errors
- [ ] Tested incrementally

---

## Resources & References

### Code Structure
```
kstuff/
├── prosper0gdb/          # Kernel access & offsets
│   ├── offsets.c         # Offset database (edit here)
│   └── r0gdb.c           # Kernel R/W primitives
├── ps5-kstuff/           # Main payload
│   ├── main.c            # Main logic (edit parasites here)
│   ├── kelf.asm          # Kernel ROP chain
│   └── uelf/             # Userland ELF loader
│       ├── main.c        # Parasite handlers
│       └── parasites.h   # Parasite logic
├── ps5-kstuff-ldr/       # Loader (sends to port 9021)
│   └── main.c            # ELF loader wrapper
└── lib/                  # Utilities
```

### Important Functions
- `r0gdb_init()` - Initialize kernel access
- `get_parasites()` - Get parasite array for firmware
- `handle_parasites()` - Check if RIP is at parasite point
- `set_offsets()` - Load offsets for firmware version

### Firmware Detection
```c
uint32_t ver = r0gdb_get_fw_version() >> 16;
// Returns: 0x960 for 9.60, 0x1001 for 10.01
```

---

## Timeline & Effort Estimate

**With 6+ years dev experience:**

| Phase | Time | Difficulty |
|-------|------|------------|
| Setup & learning | 1-2 days | Easy |
| Static analysis | 2-3 days | Medium |
| Parasite discovery | 3-5 days | Hard |
| Integration & testing | 2-3 days | Medium |
| **Total** | **1-2 weeks** | **Medium-Hard** |

**Bottleneck:** Parasite discovery (requires trial & error)

---

## Success Criteria

✅ **Complete when:**
1. All 14 parasites discovered for 10.01
2. All offsets verified
3. Payload loads without crashing
4. Can load homebrew ELF
5. System stable for 1+ hour
6. No validation errors

---

## Next Steps for You

1. **Read this document completely** - Understand the full picture
2. **Review `ADDING_10.01_SUPPORT.md`** - Technical details
3. **Use `QUICK_REFERENCE.md`** - While working
4. **Start with Phase 1** - Static analysis first
5. **Focus on Phase 2** - Parasite discovery (critical!)
6. **Test incrementally** - Don't rush
7. **Document findings** - For yourself and community

---

## Continuing This Conversation

**To continue in another LLM (ChatGPT, Claude, etc.):**

1. **Share this document** - Contains all context
2. **Mention key facts:**
   - "I have a PS5 on firmware 10.01"
   - "Using Y2JB jailbreak, can send payloads to port 9021"
   - "kstuff works on 9.60, not on 10.0x"
   - "Main issue: 14 parasite addresses need discovery"
   - "Files created: ADDING_10.01_SUPPORT.md, QUICK_REFERENCE.md, debug_instrumentation.c, discover_offsets.py"

3. **Share relevant code sections** if asking specific questions

4. **Current status:** Understanding phase, ready to start discovery

---

## Contact & Community

- **Original Author:** EchoStretch
- **Commit with 10.0x:** a1932bb (marked as "Not Working")
- **Working Version:** 9.60 (commit 70444c0)
- **Your Goal:** Make 10.01 work

---

## Final Notes

**Most Important Takeaway:**
The 10.01 support exists in the code but **doesn't work** because:
1. Parasites were copied from 10.00 (wrong addresses)
2. Need runtime discovery to find real addresses
3. Must verify ROP gadgets in Ghidra
4. Test incrementally to isolate issues

**You have everything you need:**
- ✅ Hardware (jailbroken PS5 10.01)
- ✅ Access (Y2JB + port 9021)
- ✅ Tools (documentation, scripts, instrumentation code)
- ✅ Skills (6+ years dev experience)
- ✅ Support (this comprehensive guide)

**What's needed:**
- ⏱️ Time (1-2 weeks)
- 🔍 Discovery work (find those 14 parasites!)
- 🧪 Testing (iterate and validate)

Good luck! 🚀

---

**Document Version:** 1.0  
**Created:** November 13, 2025  
**For:** PS5 10.01 kstuff Support Development  
**Status:** Ready to begin
