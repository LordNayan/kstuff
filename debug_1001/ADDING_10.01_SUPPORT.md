# Adding PS5 Firmware 10.01 Support to kstuff

## Prerequisites
- ✅ Jailbroken PS5 10.01 with Y2JB exploit
- ✅ ELF Loader that is enabled on port 9021 after Y2JB
- ✅ Debug settings enabled
- ✅ Ability to send payloads
- ✅ 6+ years dev experience
- 📦 Tools needed: Ghidra/IDA Pro, Python 3, GDB (optional)

---

## Phase 1: Kernel Dump & Static Analysis (Day 1)

### Step 1.1: Dump the Kernel Binary

Create a simple payload to dump kernel:
```c
// Use existing kstuff infrastructure to read kernel memory
// Save to /data/kernel_10.01.bin
```

**What you're looking for:** The kernel image starting at `0xffffffff80000000`

### Step 1.2: Load into Ghidra/IDA

1. Import kernel binary
2. Set architecture: x86-64
3. Set base address: `0xffffffff80000000`
4. Let auto-analysis run

---

## Phase 2: Find Basic Structure Offsets (Day 1-2)

### Critical Data Structures

Compare with 9.60 to understand what changed:

| Offset Name | 9.60 Value | 10.01 Current | How to Find |
|-------------|-----------|---------------|-------------|
| `allproc` | `0x2755d50` | `0x2765d70` | Search for string "allproc" or find process list head |
| `idt` | `0x2d94300` | `0x2d5c300` | IDT register - use SIDT instruction |
| `pcpu_array` | `0x2da8f00` | `0x2d70f00` | Per-CPU data array |
| `kernel_pmap_store` | `0x2d28b78` | `0x2cf0ef8` | Page map structure |

### How to Find These:

#### Method 1: String References
```python
# In Ghidra scripting console
currentProgram.getListing().getDefinedData(True).forEach { data ->
    if (data.hasStringValue()) {
        println(data.getAddress() + ": " + data.getValue())
    }
}
```

#### Method 2: Known Functions
Search for exported symbols like:
- `printf`
- `malloc` 
- `copyin` / `copyout`
- `syscall`

---

## Phase 3: Find ROP Gadgets (Day 2-3)

### Critical Gadgets Needed

#### 3.1: `push_pop_all_iret`
**What it does:** Saves all registers, does something, restores them, and returns via iretq

**Pattern to search for:**
```
50                push rax
51                push rcx
52                push rdx
53                push rbx
55                push rbp
56                push rsi
57                push rdi
41 50             push r8
41 51             push r9
41 52             push r10
41 53             push r11
41 54             push r12
41 55             push r13
41 56             push r14
41 57             push r15
... (some code)
58                pop rax
59                pop rcx
5A                pop rdx
5B                pop rbx
5D                pop rbp
5E                pop rsi
5F                pop rdi
41 58             pop r8
41 59             pop r9
41 5A             pop r10
41 5B             pop r11
41 5C             pop r12
41 5D             pop r13
41 5E             pop r14
41 5F             pop r15
48 CF             iretq
```

**In Ghidra:**
1. Search → For Instruction Patterns
2. Search for: `50 51 52 53`
3. Verify it ends with iretq (`48 CF`)

#### 3.2: `doreti_iret`
**Pattern:** Just find standalone `iretq` instructions:
```
48 CF             iretq
```

#### 3.3: `wrmsr_ret`
**Pattern:**
```
0F 30             wrmsr
C3                ret
```

#### 3.4: `rdmsr_start`
**Pattern:**
```
0F 32             rdmsr
```

#### 3.5: Other Gadgets
- `add_rsp_iret`: `48 83 C4 XX ... 48 CF` (add rsp, imm; ... iretq)
- `rep_movsb_pop_rbp_ret`: `F3 A4 5D C3` (rep movsb; pop rbp; ret)
- `mov_cr3_rax_mov_ds`: Move CR3 to RAX then load DS
- `mov_rax_cr3`: `48 0F 20 D8` (mov rax, cr3)

---

## Phase 4: Find Syscall Infrastructure (Day 3-4)

### 4.1: Find `sysent` Array

**What it is:** Array of syscall handlers

**How to find:**
1. Find the `syscall` handler (search for `0F 05` followed by handler)
2. Look for a large array of function pointers
3. Count should be ~600 entries

**Verification:** Entry 0 should be `nosys` or `syscall`

### 4.2: Find `syscall_before` and `syscall_after`

These are wrapper points around syscall execution.

**Pattern for syscall_before:**
```
Look for code that:
1. Saves registers
2. Calls into sysent array
3. This is where we hook to intercept
```

**In 9.60 it was:** `-0x87e101` (relative to kernel base)

**Method:**
1. Find the main syscall handler
2. Look for the wrapper that does register setup
3. Find the instruction just before calling `*sysent[rax]`

### 4.3: Find Mailbox Function

`sceSblServiceMailbox` - Used for cryptographic operations

**How to find:**
1. Search for string "sbl" or "mailbox"
2. Look for function that sends messages to security processor
3. Usually has distinctive structure with command/response

---

## Phase 5: Find Parasite Injection Points (Day 4-6) ⚠️ MOST CRITICAL

**This is why 10.01 doesn't work currently!**

Parasites are specific code locations where kstuff injects hooks to:
1. Decrypt encrypted pointers (sign-extension: `|= -1ull << 48`)
2. Intercept SELF file loading
3. Hook NPD/DRM operations

### 5.1: Understanding Parasites

Look at existing 9.60 parasites in `ps5-kstuff/main.c`:

```c
static struct PARASITES(14) parasites_960 = {
    .lim_syscall = 3,
    .lim_fself = 12,
    .lim_total = 14,
    .parasites = {
        /* syscall parasites */
        {-0x87E60E, R13},  // <-- These are RIP addresses
        {-0x3BB79C, RSI},  //     Register = which register needs fixing
        {-0x3BB75C, RSI},
        /* fself parasites */
        {-0x2F88A6, RAX},
        // ... etc
    }
};
```

### 5.2: How to Find Syscall Parasites (3 locations)

**What they do:** Fix encrypted pointers in syscall path

**Method:**
1. Set up kstuff with logging enabled
2. Modify `ps5-kstuff/uelf/main.c` around line 200:
```c
else {
    int decrypted = 0;
    #define DECRYPT(which, idx) if((regs[which] >> 48) == 0xdeb7) { 
        log_word(regs[RIP]); // LOG THIS ADDRESS!
        log_word(idx); 
        regs[which] |= 0xffffull << 48; 
        decrypted = 1; 
    }
```

3. Run various syscalls and log when decryption happens
4. The logged RIP addresses are your parasite points!

**Target syscalls to test:**
- `open()`
- `execve()`
- `mmap()`
- `socket()`

### 5.3: How to Find FSELF Parasites (9 locations)

**What they do:** Fix pointers during encrypted ELF loading

**Method:**
1. Load a signed ELF (like a game or system app)
2. Log all `0xdeb7` pointer detections
3. Filter for addresses in FSELF loading functions

**Known FSELF functions to trace:**
- `loadSelfSegment`
- `decryptSelfBlock`
- `decryptMultipleSelfBlocks`
- `sceSblAuthMgrSmIsLoadable2`

**Automated discovery approach:**

```c
// Add to uelf/main.c in the DECRYPT section:
static int parasite_count = 0;
if((regs[RIP] >> 48) == 0xffff) { // kernel address
    char msg[100];
    snprintf(msg, sizeof(msg), "Parasite %d: RIP=%llx REG=%d", 
             parasite_count++, regs[RIP], which);
    log_to_file(msg);
}
```

### 5.4: How to Find Unsorted Parasites (2 locations)

**What they do:** Fix pointers in crypto/message operations

**Location:** Usually in:
- `crypt_message_resolve`
- Mailbox handlers

**Method:** Same as above, but trigger by:
- Mounting encrypted filesystem
- Running DRM operations
- Installing PKG files

---

## Phase 6: Validation & Testing (Day 6-7)

### 6.1: Incremental Testing

Don't test everything at once! Test in stages:

**Stage 1: Basic Offsets**
```c
// Test if kernel base detection works
// Test if you can read kernel memory
// Test basic structures (allproc, idt)
```

**Stage 2: ROP Gadgets**
```c
// Test if iret chains work
// Test register save/restore
// Should not crash immediately
```

**Stage 3: Syscall Hooks**
```c
// Test syscall interception
// Try simple syscalls first
// Log everything
```

**Stage 4: Parasites**
```c
// Enable one parasite at a time
// Test with simple operations
// Gradually enable all
```

### 6.2: Debug Logging

Modify `ps5-kstuff/main.c` to add extensive logging:

```c
void debug_log(const char* msg, uint64_t val)
{
    char buf[256];
    // Format message
    // Write to /data/kstuff_debug.log
    int fd = open("/data/kstuff_debug.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
    write(fd, buf, strlen(buf));
    close(fd);
}

// Add throughout critical paths:
debug_log("Loading on CPU", cpu);
debug_log("Testing parasite", i);
debug_log("Syscall hooked", regs[RAX]);
```

---

## Phase 7: Creating the Patch (Day 7)

Once you have all offsets working:

### 7.1: Update `prosper0gdb/offsets.c`

```c
START_FW(1001)
DEF(allproc, 0xYOUR_VALUE) 
DEF(idt, 0xYOUR_VALUE)
// ... all your discovered offsets
#include "offset_list.txt"
END_FW()
```

### 7.2: Update `ps5-kstuff/main.c`

```c
static struct PARASITES(14) parasites_1001 = {
    .lim_syscall = 3,
    .lim_fself = 12,
    .lim_total = 14,
    .parasites = {
        /* syscall parasites */
        {YOUR_OFFSET_1, R13},
        {YOUR_OFFSET_2, RSI},
        {YOUR_OFFSET_3, RSI},
        /* fself parasites */
        {YOUR_OFFSET_4, RAX},
        // ... etc
    }
};
```

### 7.3: Update ShellCore Patches

```c
static struct shellcore_patch shellcore_patches_1001[] = {
    {0xADDRESS, "\xPATCH", SIZE},
    // These enable homebrew features
    // May need adjustment for 10.01
};
```

---

## Tools & Scripts

### Ghidra Script: Find ROP Gadgets

```python
# find_gadgets.py
from ghidra.program.model.address import Address

def find_pattern(pattern_bytes):
    mem = currentProgram.getMemory()
    found = mem.findBytes(
        currentProgram.getMinAddress(),
        pattern_bytes,
        None,
        True,
        monitor
    )
    results = []
    while found is not None:
        results.append(found)
        found = mem.findBytes(
            found.add(1),
            pattern_bytes,
            None,
            True,
            monitor
        )
    return results

# Find iretq instructions
iretq_pattern = [0x48, 0xcf]
results = find_pattern(iretq_pattern)
for addr in results:
    print("iretq found at: {}".format(addr))
```

### Python Script: Compare Offsets

```python
# compare_offsets.py
offsets_960 = {
    'allproc': 0x2755d50,
    'idt': 0x2d94300,
    # ... etc
}

offsets_1001 = {
    'allproc': 0x2765d70,  # YOUR DISCOVERED VALUES
    'idt': 0x2d5c300,
    # ... etc
}

print("Offset differences 9.60 -> 10.01:")
for key in offsets_960:
    diff = offsets_1001[key] - offsets_960[key]
    print(f"{key}: {hex(diff)} ({diff:+d} bytes)")
```

---

## Common Issues & Solutions

### Issue 1: Immediate Crash on Load
**Cause:** Bad ROP gadget offsets
**Solution:** Verify `doreti_iret`, `push_pop_all_iret` first

### Issue 2: Hangs During Load
**Cause:** Bad `wrmsr_ret` or infinite loop in gadget
**Solution:** Check all iret-based gadgets

### Issue 3: Works Partially Then Crashes
**Cause:** Bad parasite offsets
**Solution:** Disable all parasites, enable one-by-one

### Issue 4: Can't Load SELF Files
**Cause:** FSELF parasites wrong
**Solution:** Focus on `loadSelfSegment` family

### Issue 5: Syscalls Fail
**Cause:** Syscall hooks or `sysents` offset wrong
**Solution:** Verify syscall infrastructure first

---

## Timeline Estimate

- **Day 1-2:** Kernel dump, basic offsets (allproc, idt, sysents)
- **Day 3-4:** ROP gadgets (all iret chains, msr operations)
- **Day 4-6:** Parasite discovery (most time-consuming!)
- **Day 6-7:** Testing and validation
- **Total:** 1-2 weeks part-time

---

## Testing Checklist

- [ ] Payload loads without crashing
- [ ] Can read kernel memory
- [ ] Syscall interception works
- [ ] Can load homebrew ELF
- [ ] Can load signed SELF
- [ ] Can mount USB with homebrew
- [ ] Debug menu works
- [ ] FTP server works
- [ ] Game loading works
- [ ] Network functions work

---

## Resources

1. **FreeBSD Source**: https://github.com/freebsd/freebsd-src
   - PS5 OS is based on FreeBSD 11
   - Compare structures with kernel dump

2. **Ghidra**: https://ghidra-sre.org/
   - Best free tool for this work

3. **Existing kstuff code**:
   - `ps5-kstuff/main.c` - Main logic
   - `prosper0gdb/offsets.c` - Offset database
   - `ps5-kstuff/uelf/` - Userland ELF loader

4. **PS5 Dev Wiki** (search online)
   - Community knowledge base
   - Offset sharing

---

## Questions to Answer During Your Work

1. **Kernel Base:** Is it still `0xffffffff80000000`?
2. **KASLR:** Is address randomization enabled on 10.01?
3. **Struct Changes:** Did FreeBSD structures change size/layout?
4. **New Mitigations:** Any new security features?
5. **Syscall Numbers:** Did syscall table order change?

---

## Next Steps

1. **Start with Static Analysis**
   - Dump kernel
   - Find basic structures
   - Find ROP gadgets
   
2. **Build Instrumented Payload**
   - Add extensive logging
   - Test incrementally
   
3. **Dynamic Discovery**
   - Run real operations
   - Log parasite encounters
   - Build offset database

4. **Validate & Iterate**
   - Test each component
   - Fix issues one at a time
   
5. **Submit PR**
   - Document your findings
   - Share with community

Good luck! This is definitely doable with your background. The key is methodical testing and good logging. Start small, validate each piece, then build up.
