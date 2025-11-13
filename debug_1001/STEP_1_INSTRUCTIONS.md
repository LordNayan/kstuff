# Step 1.1: Dump PS5 Kernel Binary - Complete Instructions

## Overview
You now have the files needed to dump the PS5 kernel from firmware 10.01. This is the first critical step in adding 10.01 support to kstuff.

## Files Created
- `kernel_dumper_10.01.c` - The main payload that dumps kernel memory
- `Makefile` - Build script to compile the payload

## Prerequisites Check
Before proceeding, make sure you have:
- ✅ PS5 console running firmware 10.01
- ✅ Jailbreak active (Y2JB exploit loaded)
- ✅ ELF Loader running on port 9021 (accessible after jailbreak)
- ✅ PS5 development toolchain with clang
- ✅ Network access to your PS5
- ✅ Tool to send ELF files (like ps5-payload-sender)

## Step-by-Step Instructions

### Step 1: Compile the Payload

**Option A: Using PS5 Toolchain (Recommended)**
```bash
# Navigate to the debug_1001 directory
cd /Users/nayan/kstuff_1001/kstuff/debug_1001

# Compile using the Makefile
make

# This should create: kernel_dumper_10.01.elf
```

**Option B: Manual Compilation (if Makefile doesn't work)**
```bash
# Compile directly with clang
clang -target x86_64-scei-ps5 -O2 -Wall -Wextra kernel_dumper_10.01.c -o kernel_dumper_10.01.elf

# Or if you don't have PS5 toolchain, try generic x86-64 (might work)
clang -O2 -Wall -Wextra kernel_dumper_10.01.c -o kernel_dumper_10.01.elf
```

**Option C: Using GCC (fallback)**
```bash
gcc -O2 -Wall -Wextra kernel_dumper_10.01.c -o kernel_dumper_10.01.elf
```

### Step 2: Verify Compilation
```bash
# Check the file was created
ls -la kernel_dumper_10.01.elf

# Check file type (should be ELF x86-64)
file kernel_dumper_10.01.elf
```

You should see something like:
```
kernel_dumper_10.01.elf: ELF 64-bit LSB executable, x86-64, version 1 (SYSV)
```

### Step 3: Send Payload to PS5

**Prerequisites:**
- Your PS5 should be jailbroken and showing the Y2JB success screen
- ELF Loader should be running (usually automatic after Y2JB)
- Note your PS5's IP address

**Method A: Using ps5-payload-sender (if you have it)**
```bash
# Replace 192.168.1.XXX with your PS5's IP
ps5-payload-sender -ip 192.168.1.XXX -file kernel_dumper_10.01.elf
```

**Method B: Using netcat (manual method)**
```bash
# Replace 192.168.1.XXX with your PS5's IP
nc 192.168.1.XXX 9021 < kernel_dumper_10.01.elf
```

**Method C: Using Python script**
```python
import socket

# Replace with your PS5's IP
PS5_IP = "192.168.1.XXX"
PS5_PORT = 9021

with open("kernel_dumper_10.01.elf", "rb") as f:
    payload = f.read()

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((PS5_IP, PS5_PORT))
sock.send(payload)
sock.close()

print("Payload sent!")
```

### Step 4: Execute and Monitor

**What to expect:**
1. The payload should execute automatically after sending
2. You might see output on your PS5 screen (if debug console is enabled)
3. The payload will create `/data/kernel_10.01.bin` on your PS5

**Timeline:**
- The dump should take 1-3 minutes depending on kernel size
- You'll see progress updates if debug output is visible

### Step 5: Verify the Dump

**On PS5 (if you have shell access):**
```bash
# Check if the dump was created
ls -la /data/kernel_10.01.bin

# Check file size (should be 16-32 MB)
du -h /data/kernel_10.01.bin

# Check first few bytes (should start with ELF magic)
hexdump -C /data/kernel_10.01.bin | head
```

**Expected output:**
```
00000000  7f 45 4c 46 02 01 01 09  00 00 00 00 00 00 00 00  |.ELF............|
```

The first 4 bytes should be `7f 45 4c 46` (ELF magic).

### Step 6: Copy Dump to Analysis Machine

**Method A: Using FTP (if PS5 FTP server is running)**
```bash
# Connect to PS5 FTP (usually port 21)
ftp 192.168.1.XXX

# Navigate to data directory
cd /data

# Download the kernel dump
get kernel_10.01.bin

# Exit FTP
quit
```

**Method B: Using USB (copy to USB drive on PS5)**
1. Insert USB drive into PS5
2. Use PS5 file manager to copy `/data/kernel_10.01.bin` to USB
3. Move USB to your analysis machine

**Method C: Using network share**
If you have network sharing set up, copy the file from `/data/` to your shared folder.

## Troubleshooting

### Problem: Compilation Fails
**Error:** `clang: command not found` or similar
**Solution:** 
- Install PS5 development tools
- Or try with regular GCC: `gcc -O2 kernel_dumper_10.01.c -o kernel_dumper_10.01.elf`

### Problem: Can't Send to PS5
**Error:** Connection refused on port 9021
**Solution:**
- Make sure Y2JB jailbreak is active
- Check if ELF Loader is running (should start automatically)
- Verify PS5 IP address
- Try restarting the jailbreak process

### Problem: Payload Crashes PS5
**Error:** PS5 reboots or hangs after sending payload
**Possible causes:**
- Kernel memory layout has changed significantly
- Security mitigations prevent kernel access
- Payload has bugs

**Solutions:**
- Check kstuff is loaded first
- Try a simpler test payload that just prints "Hello World"
- Make sure you're on actual 10.01 firmware

### Problem: No Output File Created
**Error:** `/data/kernel_10.01.bin` doesn't exist
**Possible causes:**
- Payload didn't execute
- No write permissions to `/data/`
- Kernel access blocked

**Solutions:**
- Check PS5 debug console for error messages
- Try writing to a different location (like `/tmp/`)
- Verify jailbreak is working properly

### Problem: Dump File is Wrong Size
**Error:** File is too small (< 10MB) or too large (> 50MB)
**Solutions:**
- Check if dump was truncated (disk space?)
- Verify kernel base address hasn't changed
- Look at hex dump to see if it contains valid ELF data

## Success Criteria

You've successfully completed Step 1.1 if:
- ✅ `kernel_dumper_10.01.elf` compiles without errors
- ✅ Payload sends to PS5 without connection issues
- ✅ `/data/kernel_10.01.bin` is created on PS5
- ✅ File size is reasonable (16-32 MB)
- ✅ File starts with ELF magic bytes (`7F 45 4C 46`)
- ✅ You can copy the dump to your analysis machine

## Next Steps

Once you have a successful kernel dump:
1. **Load into Ghidra/IDA** with base address `0xffffffff80000000`
2. **Let auto-analysis complete** (may take 30+ minutes)
3. **Start Phase 2**: Finding basic structure offsets
4. **Begin comparing** with existing 9.60 offsets

## Files to Keep

Save these files for documentation:
- `kernel_dumper_10.01.c` - Your dumper source
- `kernel_dumper_10.01.elf` - Compiled payload  
- `kernel_10.01.bin` - The actual kernel dump (most important!)
- Build logs and any error messages

## Security Note

The kernel dump contains sensitive system information. Keep it secure and don't share it publicly unless you're sure it doesn't contain any personally identifiable information.

---

**Ready for Next Phase?**
Once you have a valid kernel dump, you can proceed to Phase 2: Finding Basic Structure Offsets in your main guide.