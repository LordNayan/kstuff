#!/bin/bash
set -e

# Find all symlinks in freebsd-headers and remove them from git
HEADER_DIR="freebsd-headers"

find "$HEADER_DIR" -type l | while read -r symlink; do
    echo "Removing symlink from git: $symlink"
    git rm "$symlink"
done

echo "All symlinks removed from git index."
