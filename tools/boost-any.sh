#!/bin/bash

if [ -z "$1" ]; then
    echo "Usage: $0 <repo_root>"
    exit 1
fi

REPO_ROOT="$1"

if [ ! -d "$REPO_ROOT" ]; then
    echo "Error: Directory $REPO_ROOT does not exist"
    exit 1
fi

cd "$REPO_ROOT"

echo "Starting boost::any to std::any migration..."

echo "Step 1/4: Replacing boost::any with std::any..."
comby 'boost::any' 'std::any' .cpp,.h,.hpp -d .

echo "Step 2/4: Replacing boost::any_cast with std::any_cast..."
comby 'boost::any_cast' 'std::any_cast' .cpp,.h,.hpp -d .

echo "Step 3/4: Removing #include <boost/any.hpp> statements..."
comby '#include <boost/any.hpp>' '' .cpp,.h,.hpp -d .

echo "Step 4/4: Adding #include <any> to files that use std::any..."
find . -name "*.cpp" -o -name "*.h" -o -name "*.hpp" | xargs grep -l "std::any" | while read file; do
    if ! grep -q "#include <any>" "$file"; then
        echo "  Adding #include <any> to $file"
        sed -i '1i#include <any>' "$file"
    else
        echo "  $file already has #include <any>"
    fi
done

echo "Migration complete!"
