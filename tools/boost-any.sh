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

comby 'boost::any' 'std::any' .cpp,.h,.hpp -d .

comby 'boost::any_cast' 'std::any_cast' .cpp,.h,.hpp -d .

comby '#include <boost/any.hpp>' '' .cpp,.h,.hpp -d .

find . -name "*.cpp" -o -name "*.h" -o -name "*.hpp" | xargs grep -l "std::any" | while read file; do
    if ! grep -q "#include <any>" "$file"; then
        sed -i '1i#include <any>' "$file"
    fi
done
