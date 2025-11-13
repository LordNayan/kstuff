# Makefile for PS5 kernel dumper
# Requires PS5 development toolchain

# Compiler settings for PS5
CC=clang
CFLAGS=-target x86_64-scei-ps5 -O2 -Wall -Wextra
LDFLAGS=-target x86_64-scei-ps5

# Source and output files
SRC=kernel_dumper_10.01.c
TARGET=kernel_dumper_10.01.elf

# Default target
all: $(TARGET)

# Compile the kernel dumper
$(TARGET): $(SRC)
	@echo "Compiling PS5 kernel dumper..."
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)
	@echo "Compilation complete: $(TARGET)"
	@ls -la $(TARGET)

# Clean build files
clean:
	rm -f $(TARGET)
	@echo "Cleaned build files"

# Install target - copy to PS5 payload directory
install: $(TARGET)
	@echo "Install target not implemented - manually copy $(TARGET) to your PS5 payload sender"

# Help target
help:
	@echo "PS5 Kernel Dumper Build Instructions"
	@echo "===================================="
	@echo ""
	@echo "Prerequisites:"
	@echo "  - PS5 development toolchain installed"
	@echo "  - clang with PS5 target support"
	@echo ""
	@echo "Targets:"
	@echo "  all     - Build the kernel dumper (default)"
	@echo "  clean   - Remove build files"
	@echo "  help    - Show this help message"
	@echo ""
	@echo "Usage:"
	@echo "  make                    # Build the dumper"
	@echo "  make clean             # Clean build files"
	@echo ""
	@echo "After building:"
	@echo "  1. Send $(TARGET) to PS5 via ELF Loader (port 9021)"
	@echo "  2. Execute the payload on your jailbroken PS5"
	@echo "  3. Check /data/kernel_10.01.bin for the dump"

.PHONY: all clean install help