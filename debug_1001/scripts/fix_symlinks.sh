#!/bin/bash
set -e

# Find the root of the repository dynamically
REPO_ROOT=$(git rev-parse --show-toplevel 2>/dev/null)

if [ -z "$REPO_ROOT" ]; then
    echo "Error: Not in a git repository"
    exit 1
fi

HEADER_DIR="$REPO_ROOT/freebsd-headers"

if [ ! -d "$HEADER_DIR" ]; then
    echo "Error: Header directory not found at $HEADER_DIR"
    exit 1
fi

find "$HEADER_DIR" -type f | while read -r file; do
    # Read the first line
    first_line=$(head -n 1 "$file")
    # Check if it matches a symlink stub (e.g., sys/foo.h or machine/bar.h)
    if [[ "$first_line" =~ ^[a-zA-Z0-9_/.-]+\.h$ ]]; then
        target="$HEADER_DIR/$first_line"
        if [[ -f "$target" ]]; then
            echo "Fixing symlink stub: $file -> $target"
            cp "$target" "$file"
        else
            echo "Target not found for $file: $target"
        fi
    fi
done

echo "Symlink stubs fixed."