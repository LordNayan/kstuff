#!/bin/bash
# Find the root of the repository dynamically
REPO_ROOT=$(git rev-parse --show-toplevel 2>/dev/null)

if [ -z "$REPO_ROOT" ]; then
    echo "Error: Not in a git repository"
    exit 1
fi

# Change to ps5-kstuff directory and run make clean
if [ -d "$REPO_ROOT/ps5-kstuff" ]; then
    echo "Cleaning ps5-kstuff directory..."
    cd "$REPO_ROOT/ps5-kstuff" && make clean
else
    echo "Warning: ps5-kstuff directory not found"
fi

# Change to lib directory and run make clean
if [ -d "$REPO_ROOT/lib" ]; then
    echo "Cleaning lib directory..."
    cd "$REPO_ROOT/lib" && make clean
else
    echo "Warning: lib directory not found"
fi

# Change to prosper0gdb directory and run make clean
if [ -d "$REPO_ROOT/prosper0gdb" ]; then
    echo "Cleaning prosper0gdb directory..."
    cd "$REPO_ROOT/prosper0gdb" && make clean
else
    echo "Warning: prosper0gdb directory not found"
fi

echo "Done cleaning output files"