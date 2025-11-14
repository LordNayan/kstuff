# Quick Reference: Adding 10.01 Support

## Prerequisites ✅
- [x] PS5 on firmware 10.01
- [x] Y2JB jailbreak working
- [x] Debug settings enabled
- [x] Can send payloads via network

## Phase 1: Static Analysis (2-3 hours)

### Task 1.1: Dump Kernel
```bash
# Create simple memory dump payload
# Target: /boot/kernel/kernel from physical console
# Or use Y2JB to dump kernel memory region
```

### Task 1.2: Load into Ghidra
1. File → Import File → kernel_10.01.bin
2. Architecture: x86:LE:64:default
3. Base Address: 0xffffffff80000000
4. Let analysis complete (~5-10 min)

### Task 1.3: Find Basic Offsets
| Offset | How to Find | Status |
|--------|-------------|--------|
| `allproc` | Search string "allproc" | ⬜ |
| `idt` | Search for SIDT usage | ⬜ |
| `sysents` | Search "syscall" → find array | ⬜ |
| `malloc` | Symbol or xref to "M_TEMP" | ⬜ |

## Phase 2: ROP Gadgets (3-4 hours)

### Critical Gadgets Checklist

#### doreti_iret
- [ ] Search hex: `48 CF` (iretq)
- [ ] Verify it's in kernel return path
- [ ] Test address: _____________

#### push_pop_all_iret  
- [ ] Search hex: `50 51 52 53 55 56 57` (push all)
- [ ] Verify ends with `48 CF` (iretq)
- [ ] **CRITICAL**: This is different between 10.00 and 10.01!
- [ ] Test address: _____________

#### wrmsr_ret
- [ ] Search hex: `0F 30 C3` (wrmsr; ret)
- [ ] Test address: _____________

#### rdmsr_start
- [ ] Search hex: `0F 32` (rdmsr)
- [ ] Test address: _____________

#### rep_movsb_pop_rbp_ret
- [ ] Search hex: `F3 A4 5D C3`
- [ ] Test address: _____________

## Phase 3: Parasite Discovery (MOST IMPORTANT - 8-12 hours)

### Setup
1. [ ] Copy `debug_instrumentation.c` code into `ps5-kstuff/main.c`
2. [ ] Build with `-DDISCOVER_MODE`
3. [ ] Deploy to PS5

### Discovery Process

#### Round 1: Syscall Parasites (3 needed)
- [ ] Run test: `open("/system/common/lib/libc.sprx")`
- [ ] Check `/data/kstuff_parasites.log`
- [ ] Found addresses: ___________, ___________, ___________

#### Round 2: FSELF Parasites (9 needed)
- [ ] Load homebrew ELF
- [ ] Load system SELF file
- [ ] Check log for "DISCOVERED" entries in FSELF region
- [ ] Found addresses: 
  - [ ] ___________ (RAX)
  - [ ] ___________ (RAX)
  - [ ] ___________ (RDX)
  - [ ] ___________ (RAX)
  - [ ] ___________ (R10)
  - [ ] ___________ (RAX)
  - [ ] ___________ (RAX)
  - [ ] ___________ (RDI)
  - [ ] ___________ (RAX)

#### Round 3: Unsorted Parasites (2 needed)
- [ ] Trigger crypto operations
- [ ] Mount filesystem
- [ ] Found addresses: ___________, ___________

### Validation
- [ ] No duplicate addresses
- [ ] All addresses in kernel range (0xffffffff........)
- [ ] Tested with real operations
- [ ] No crashes when hit

## Phase 4: Integration (2-3 hours)

### Update offsets.c
```c
START_FW(1001)
DEF(allproc, 0x_____________)
DEF(idt, 0x_____________)
DEF(push_pop_all_iret, -0x_____________)  // VERIFY THIS!
// ... all others
#include "offset_list.txt"
END_FW()
```

### Update main.c parasites
```c
static struct PARASITES(14) parasites_1001 = {
    .lim_syscall = 3,
    .lim_fself = 12,
    .lim_total = 14,
    .parasites = {
        /* YOUR DISCOVERED ADDRESSES HERE */
    }
};
```

### Update case statement
```c
case 0x1001: set_offsets_1001(); break;  // In offsets.c
case 0x1001:                              // In main.c get_parasites()
    *desc_size = sizeof(parasites_1001);
    return (void*)&parasites_1001;
```

## Phase 5: Testing (2-3 hours)

### Test Progression
1. [ ] Payload loads without crash
2. [ ] Can read kernel memory  
3. [ ] Debug menu accessible
4. [ ] Can load homebrew ELF
5. [ ] Can run FTP server
6. [ ] Can load games
7. [ ] System stable after 1 hour

### If It Crashes...

| Symptom | Likely Cause | Fix |
|---------|--------------|-----|
| Instant crash | Bad `doreti_iret` | Re-find in Ghidra |
| Hang on load | Bad `push_pop_all_iret` | Critical - must be exact |
| Syscalls fail | Wrong `sysents` offset | Check syscall table |
| Can't load ELF | Bad FSELF parasites | Re-discover |
| Crashes after working | Wrong unsorted parasites | Test individually |

## Tools

### Python Helper
```bash
cd /Users/nayan/kstuff
python3 discover_offsets.py --mode compare   # Show differences
python3 discover_offsets.py --mode validate  # Check for issues
python3 discover_offsets.py --mode generate  # Generate C code
```

### Ghidra Script (find_gadgets.py)
```python
# Search → For Instruction Patterns
# Enter: 48 CF (iretq)
# Or use script to automate
```

## Success Criteria

- [x] All offsets found
- [x] All 14 parasites discovered  
- [x] No validation errors
- [x] Tested on real PS5
- [x] Runs homebrew successfully
- [x] Stable for 1+ hour
- [x] Ready for PR

## Time Estimate
- **Fastest**: 1-2 days (if everything works first try)
- **Realistic**: 1 week
- **With issues**: 2 weeks

## Resources
- This guide: `ADDING_10.01_SUPPORT.md`
- Debug code: `debug_instrumentation.c`
- Helper script: `discover_offsets.py`
- Offset finder: `offset_finder_10.01.c`

## Common Mistakes to Avoid
1. ❌ Copying 10.00 offsets directly
2. ❌ Not validating each offset
3. ❌ Skipping parasite discovery
4. ❌ Testing everything at once
5. ❌ Not logging enough

## Pro Tips
1. ✅ Test incrementally - one component at a time
2. ✅ Log EVERYTHING during discovery
3. ✅ Compare with 9.60 to understand patterns
4. ✅ Validate offsets with static analysis before testing
5. ✅ Keep good notes of what works/doesn't work

## Getting Help
- Check existing firmware support in the code
- Look at how 9.60 differs from 9.00
- Compare offset patterns between versions
- Test one parasite at a time to isolate issues

## Notes Section
_Use this space to track your discoveries:_

```
Date: __________
Attempt: __________
Results:

Issues found:

Next steps:
```

---

**Good luck! You can do this. Take it step by step, log everything, and test incrementally.**
