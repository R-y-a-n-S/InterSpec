#!/usr/bin/env bash

# Script to replace boost::any with std::any and boost::any_cast with std::any_cast
# Also removes boost/any.hpp includes and adds std <any> includes where needed

set -e

# Validate arguments
if [ $# -ne 1 ]; then
    echo "Usage: $0 <repo_root>"
    echo "  Replaces boost::any with std::any across the repository"
    exit 1
fi

REPO_ROOT="$1"
SCRIPT_NAME=$(basename "$0")

# Validate repo root exists and is a directory
if [ ! -d "$REPO_ROOT" ]; then
    echo "Error: '$REPO_ROOT' is not a directory"
    echo "Usage: $0 <repo_root>"
    exit 1
fi

echo "Starting boost::any to std::any transformation in: $REPO_ROOT"
echo "Excluding script: $SCRIPT_NAME"

# Step 1: Replace boost::any with std::any
echo "Step 1: Replacing boost::any with std::any..."
comby -matcher .txt -in-place -d "$REPO_ROOT" -exclude "$SCRIPT_NAME" \
    'boost::any' 'std::any'

# Step 2: Replace boost::any_cast with std::any_cast
echo "Step 2: Replacing boost::any_cast with std::any_cast..."
comby -matcher .txt -in-place -d "$REPO_ROOT" -exclude "$SCRIPT_NAME" \
    'boost::any_cast' 'std::any_cast'

# Step 3: Remove boost/any.hpp includes
echo "Step 3: Removing #include <boost/any.hpp>..."
comby -matcher .txt -in-place -d "$REPO_ROOT" -exclude "$SCRIPT_NAME" \
    '#include <boost/any.hpp>' ''

# Step 4: Add std <any> include where std::any is used but <any> is not included
echo "Step 4: Adding #include <any> where needed..."
# First find files that use std::any
echo "  Finding files that use std::any..."
FILES_WITH_STD_ANY=$(grep -r -l "std::any" "$REPO_ROOT" --exclude="$SCRIPT_NAME" || true)

if [ -n "$FILES_WITH_STD_ANY" ]; then
    for file in $FILES_WITH_STD_ANY; do
        # Check if file already has #include <any>
        if ! grep -q "#include <any>" "$file"; then
            echo "  Adding #include <any> to: $file"
            # Add #include <any> after the last #include line
            comby -matcher .txt -in-place -f "$file" \
                ':[last_include]#include :[~include_path]' \
                ':[last_include]#include :[include_path]
#include <any>'
        fi
    done
fi

echo "Transformation complete!"
echo "Summary of changes:"
echo "  - Replaced boost::any with std::any"
echo "  - Replaced boost::any_cast with std::any_cast" 
echo "  - Removed #include <boost/any.hpp>"
echo "  - Added #include <any> where needed"