#!/usr/bin/env python3
"""
PS5 10.01 Offset Discovery Helper

This script helps organize and validate offsets discovered from
reverse engineering the PS5 10.01 kernel.

Usage:
    python3 discover_offsets.py --mode compare    # Compare with 9.60
    python3 discover_offsets.py --mode validate   # Validate offsets
    python3 discover_offsets.py --mode generate   # Generate C code
"""

import sys
import argparse

# Known working offsets from 9.60
OFFSETS_960 = {
    # Data structures
    'allproc': 0x2755d50,
    'idt': 0x2d94300,
    'gdt_array': 0x2d955e0,
    'tss_array': 0x2d96fe0,
    'pcpu_array': 0x2da8f00,
    
    # ROP gadgets
    'doreti_iret': -0xa52e93,
    'push_pop_all_iret': -0x9f36c8,
    'rep_movsb_pop_rbp_ret': -0xa167e6,
    'rdmsr_start': -0xa545ca,
    'wrmsr_ret': -0xa5599c,
    
    # Syscall infrastructure
    'syscall_before': -0x87e101,
    'syscall_after': -0x87e0dd,
    'sysents': 0x1aac60,
    'sysents_ps4': 0x1a2650,
    
    # Crypto/security
    'sceSblServiceMailbox': -0x6e77d0,
    'sceSblAuthMgrSmIsLoadable2': -0x928b80,
    'copyin': -0xa170b0,
    'copyout': -0xa17160,
    
    # Other critical
    'malloc': -0xbca60,
    'kernel_pmap_store': 0x2d28b78,
}

# Placeholder for 10.01 offsets (YOU FILL THESE IN)
OFFSETS_1001 = {
    # Data structures  
    'allproc': 0x2765d70,  # VERIFY THIS
    'idt': 0x2d5c300,      # VERIFY THIS
    'gdt_array': 0x2d5d5e0,
    'tss_array': 0x2d5efe0,
    'pcpu_array': 0x2d70f00,
    
    # ROP gadgets - THESE ARE CRITICAL
    'doreti_iret': -0xa6eb13,
    'push_pop_all_iret': -0xa10540,  # Different from 10.00! Verify!
    'rep_movsb_pop_rbp_ret': -0xa32466,
    'rdmsr_start': -0xa7024a,
    'wrmsr_ret': -0xa7161c,
    
    # Syscall infrastructure
    'syscall_before': -0x893e21,  # VERIFY THIS
    'syscall_after': -0x893ded,   # VERIFY THIS
    'sysents': 0x1ad100,
    'sysents_ps4': 0x1a4bb0,
    
    # Crypto/security - VERIFY ALL OF THESE
    'sceSblServiceMailbox': -0x6f8b10,
    'sceSblAuthMgrSmIsLoadable2': -0x941160,
    'copyin': -0xa32d30,
    'copyout': -0xa32de0,
    
    # Other critical
    'malloc': -0xbb850,
    'kernel_pmap_store': 0x2cf0ef8,
}

# Parasites for 10.01 (YOU MUST DISCOVER THESE)
PARASITES_1001 = {
    'syscall': [
        {'offset': -0x894308, 'reg': 'R13'},  # UNVERIFIED - likely wrong!
        {'offset': -0x3BF480, 'reg': 'RSI'},  # UNVERIFIED - likely wrong!
        {'offset': -0x3BF440, 'reg': 'RSI'},  # UNVERIFIED - likely wrong!
    ],
    'fself': [
        {'offset': -0x2FC476, 'reg': 'RAX'},  # UNVERIFIED - likely wrong!
        {'offset': -0x2FCFDA, 'reg': 'RAX'},
        {'offset': -0x2FCE96, 'reg': 'RDX'},
        {'offset': -0x2FCC0B, 'reg': 'RAX'},
        {'offset': -0x2FC932, 'reg': 'R10'},
        {'offset': -0x2FC5FA, 'reg': 'RAX'},
        {'offset': -0x2FC5EE, 'reg': 'RAX'},
        {'offset': -0xA32F8C, 'reg': 'RDI'},
        {'offset': -0x2FCA77, 'reg': 'RAX'},
    ],
    'unsorted': [
        {'offset': -0x4B5766, 'reg': 'RCX'},
        {'offset': -0x4B5766, 'reg': 'R14'},
    ]
}

REG_MAP = {
    'RAX': 0, 'RCX': 1, 'RDX': 2, 'RBX': 3,
    'RSP': 4, 'RBP': 5, 'RSI': 6, 'RDI': 7,
    'R8': 8, 'R9': 9, 'R10': 10, 'R11': 11,
    'R12': 12, 'R13': 13, 'R14': 14, 'R15': 15
}

def compare_offsets():
    """Compare 9.60 and 10.01 offsets to find patterns"""
    print("=" * 80)
    print("OFFSET COMPARISON: 9.60 vs 10.01")
    print("=" * 80)
    print()
    
    print(f"{'Offset Name':<35} {'9.60':<15} {'10.01':<15} {'Difference':<15}")
    print("-" * 80)
    
    for key in sorted(OFFSETS_960.keys()):
        val_960 = OFFSETS_960[key]
        val_1001 = OFFSETS_1001.get(key, 0)
        
        if val_1001 == 0:
            diff_str = "MISSING!"
            status = "❌"
        else:
            diff = val_1001 - val_960
            diff_str = f"{diff:+d} (0x{diff:+x})"
            
            # Flag suspicious differences
            if abs(diff) > 0x100000:
                status = "⚠️"
            else:
                status = "✓"
        
        print(f"{status} {key:<33} {hex(val_960):<15} {hex(val_1001) if val_1001 else 'N/A':<15} {diff_str}")
    
    print()
    print("Legend:")
    print("  ✓  = Reasonable difference")
    print("  ⚠️  = Large difference (verify carefully)")
    print("  ❌ = Missing offset")

def validate_offsets():
    """Validate offsets for common issues"""
    print("=" * 80)
    print("OFFSET VALIDATION")
    print("=" * 80)
    print()
    
    errors = []
    warnings = []
    
    # Check for zero offsets
    for key, val in OFFSETS_1001.items():
        if val == 0:
            errors.append(f"❌ {key} is 0 (not set)")
    
    # Check ROP gadget alignment
    gadgets = ['doreti_iret', 'push_pop_all_iret', 'wrmsr_ret']
    for gadget in gadgets:
        if gadget in OFFSETS_1001:
            addr = OFFSETS_1001[gadget]
            if addr > 0:
                errors.append(f"❌ {gadget} should be negative (relative to kernel base)")
    
    # Check if push_pop_all_iret is different from 10.00
    if OFFSETS_1001.get('push_pop_all_iret') == -0xa106b8:
        warnings.append(f"⚠️  push_pop_all_iret matches 10.00 exactly - this is suspicious!")
    
    # Check parasite duplicates
    all_parasite_addrs = []
    for category in PARASITES_1001.values():
        for p in category:
            all_parasite_addrs.append(p['offset'])
    
    if len(all_parasite_addrs) != len(set(all_parasite_addrs)):
        warnings.append("⚠️  Duplicate parasite addresses found")
    
    # Check if parasites are identical to 10.00
    suspected_10_00_parasites = [
        -0x894308, -0x3BF480, -0x3BF440,
        -0x2FC476, -0x2FCFDA, -0x2FCE96
    ]
    matches = sum(1 for addr in all_parasite_addrs if addr in suspected_10_00_parasites)
    if matches >= 5:
        errors.append(f"❌ Too many parasites ({matches}) match 10.00 - likely not discovered properly!")
    
    # Print results
    if errors:
        print("ERRORS:")
        for err in errors:
            print(f"  {err}")
        print()
    
    if warnings:
        print("WARNINGS:")
        for warn in warnings:
            print(f"  {warn}")
        print()
    
    if not errors and not warnings:
        print("✅ All validation checks passed!")
    else:
        print(f"\nSummary: {len(errors)} errors, {len(warnings)} warnings")
        if errors:
            print("\n⚠️  DO NOT USE THESE OFFSETS YET - FIX ERRORS FIRST!")

def generate_c_code():
    """Generate C code for offsets.c and main.c"""
    print("=" * 80)
    print("GENERATED C CODE FOR prosper0gdb/offsets.c")
    print("=" * 80)
    print()
    
    print("START_FW(1001)")
    for key in sorted(OFFSETS_1001.keys()):
        val = OFFSETS_1001[key]
        if isinstance(val, int):
            print(f"DEF({key}, {hex(val)})")
    print("#include \"offset_list.txt\"")
    print("END_FW()")
    
    print()
    print("=" * 80)
    print("GENERATED C CODE FOR ps5-kstuff/main.c (parasites)")
    print("=" * 80)
    print()
    
    total = (len(PARASITES_1001['syscall']) + 
             len(PARASITES_1001['fself']) + 
             len(PARASITES_1001['unsorted']))
    
    print(f"static struct PARASITES({total}) parasites_1001 = {{")
    print(f"    .lim_syscall = {len(PARASITES_1001['syscall'])},")
    print(f"    .lim_fself = {len(PARASITES_1001['syscall']) + len(PARASITES_1001['fself'])},")
    print(f"    .lim_total = {total},")
    print("    .parasites = {")
    
    print("        /* syscall parasites */")
    for p in PARASITES_1001['syscall']:
        print(f"        {{{hex(p['offset'])}, {p['reg']}}},")
    
    print("        /* fself parasites */")
    for p in PARASITES_1001['fself']:
        print(f"        {{{hex(p['offset'])}, {p['reg']}}},")
    
    print("        /* unsorted parasites */")
    for p in PARASITES_1001['unsorted']:
        print(f"        {{{hex(p['offset'])}, {p['reg']}}},")
    
    print("    }")
    print("};")

def interactive_mode():
    """Interactive offset entry"""
    print("=" * 80)
    print("INTERACTIVE OFFSET DISCOVERY")
    print("=" * 80)
    print()
    print("Enter offsets as you discover them.")
    print("Format: <name> <hex_value>")
    print("Example: allproc 0x2765d70")
    print("Type 'done' when finished, 'show' to display current offsets")
    print()
    
    discovered = {}
    
    while True:
        try:
            inp = input("> ").strip()
            
            if inp.lower() == 'done':
                break
            elif inp.lower() == 'show':
                print("\nCurrent discoveries:")
                for k, v in discovered.items():
                    print(f"  {k}: {hex(v)}")
                print()
                continue
            
            parts = inp.split()
            if len(parts) != 2:
                print("Invalid format. Use: <name> <hex_value>")
                continue
            
            name = parts[0]
            try:
                value = int(parts[1], 16) if parts[1].startswith('0x') else int(parts[1])
                discovered[name] = value
                print(f"✓ Added {name} = {hex(value)}")
            except ValueError:
                print("Invalid hex value")
        
        except EOFError:
            break
    
    print("\nDiscovered offsets:")
    for k, v in sorted(discovered.items()):
        print(f"'{k}': {hex(v)},")

def main():
    parser = argparse.ArgumentParser(description='PS5 10.01 Offset Discovery Helper')
    parser.add_argument('--mode', choices=['compare', 'validate', 'generate', 'interactive'],
                       default='compare', help='Operation mode')
    
    args = parser.parse_args()
    
    if args.mode == 'compare':
        compare_offsets()
    elif args.mode == 'validate':
        validate_offsets()
    elif args.mode == 'generate':
        generate_c_code()
    elif args.mode == 'interactive':
        interactive_mode()

if __name__ == '__main__':
    main()
