#!/usr/bin/env bash
#
# Usage: ./tools/boost-any.sh <repo-root>   (defaults to current dir)
# Runs a comby codemod ONLY on C/C++ source files, skipping build + VCS dirs.

set -euo pipefail
ROOT="${1:-.}"

# 1. Build a file list -----------------------------------------------
tmpfile="$(mktemp)"
find "$ROOT" \
  -type f \( -name '*.cpp' -o -name '*.cc' -o -name '*.cxx' \
             -o -name '*.hpp' -o -name '*.hh' -o -name '*.h' \) \
  -not -path '*/build/*'        \
  -not -path '*/out/*'          \
  -not -path '*/.git/*'         \
  -not -path '*/third_party/*'  \
  > "$tmpfile"

# 2. Run replacements -------------------------------------------------
echo "▶ Replacing boost::any → std::any …"
comby -in-place -f "$tmpfile" 'boost::any' 'std::any' -matcher .generic

echo "▶ Replacing boost::any_cast → std::any_cast …"
comby -in-place -f "$tmpfile" 'boost::any_cast' 'std::any_cast' -matcher .generic

echo "▶ Removing old include and adding <any> where needed …"
# Drop the boost header
sed -i '/#include[[:space:]]*<boost\/any\.hpp>/d' $(cat "$tmpfile")

# Prepend <any> if still missing
grep -L '#include <any>' $(cat "$tmpfile") \
  | xargs -r sed -i '1i#include <any>'

rm "$tmpfile"
echo "✅ Codemod complete."
