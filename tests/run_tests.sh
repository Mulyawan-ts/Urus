#!/bin/bash
# URUS Test Runner
# Usage: ./run_tests.sh [path/to/urusc]

URUSC="${1:-../compiler/urusc}"
PASS=0
FAIL=0
SKIP=0

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m'

echo "=== URUS Test Suite ==="
echo ""

# --- Valid compilation tests ---
echo "--- Valid programs (should compile) ---"
for f in valid/*.urus; do
    [ -f "$f" ] || continue
    name=$(basename "$f" .urus)
    if "$URUSC" "$f" --emit-c > /dev/null 2>&1; then
        echo -e "  ${GREEN}PASS${NC} $name"
        PASS=$((PASS + 1))
    else
        echo -e "  ${RED}FAIL${NC} $name (compilation failed)"
        FAIL=$((FAIL + 1))
    fi
done

echo ""

# --- Invalid programs (should produce error) ---
echo "--- Invalid programs (should reject) ---"
for f in invalid/*.urus; do
    [ -f "$f" ] || continue
    name=$(basename "$f" .urus)
    if "$URUSC" "$f" --emit-c > /dev/null 2>&1; then
        echo -e "  ${RED}FAIL${NC} $name (should have been rejected)"
        FAIL=$((FAIL + 1))
    else
        echo -e "  ${GREEN}PASS${NC} $name"
        PASS=$((PASS + 1))
    fi
done

echo ""

# --- Run tests (compile + run + check output) ---
echo "--- Run tests (compile, run, check output) ---"
for f in run/*.urus; do
    [ -f "$f" ] || continue
    name=$(basename "$f" .urus)
    expected="run/${name}.expected"
    if [ ! -f "$expected" ]; then
        echo -e "  ${YELLOW}SKIP${NC} $name (no .expected file)"
        SKIP=$((SKIP + 1))
        continue
    fi

    # Determine executable extension
    exe_ext=""
    case "$(uname -s)" in
        MINGW*|MSYS*|CYGWIN*|*_NT*) exe_ext=".exe" ;;
    esac

    # Compile: use --emit-c and compile the C output with gcc directly
    # This avoids system()/cmd.exe issues on Windows
    c_tmp="run/${name}_tmp.c"
    if ! "$URUSC" "$f" --emit-c > "$c_tmp" 2>/dev/null; then
        echo -e "  ${RED}FAIL${NC} $name (urus compilation failed)"
        rm -f "$c_tmp"
        FAIL=$((FAIL + 1))
        continue
    fi

    # Find include directory (relative to urusc)
    URUSC_DIR=$(dirname "$URUSC")
    INCLUDE_DIR="${URUSC_DIR}/include"

    if ! gcc -std=c11 -O2 -I "$INCLUDE_DIR" -o "run/${name}${exe_ext}" "$c_tmp" -lm 2>/dev/null; then
        echo -e "  ${RED}FAIL${NC} $name (C compilation failed)"
        rm -f "$c_tmp"
        FAIL=$((FAIL + 1))
        continue
    fi
    rm -f "$c_tmp"

    # Run and compare (strip \r for Windows compatibility)
    actual=$("run/${name}${exe_ext}" 2>&1 | tr -d '\r')
    expected_content=$(cat "$expected" | tr -d '\r')
    if [ "$actual" = "$expected_content" ]; then
        echo -e "  ${GREEN}PASS${NC} $name"
        PASS=$((PASS + 1))
    else
        echo -e "  ${RED}FAIL${NC} $name (output mismatch)"
        echo "    Expected: $(head -1 "$expected")..."
        echo "    Got:      $(echo "$actual" | head -1)..."
        FAIL=$((FAIL + 1))
    fi

    # Cleanup
    rm -f "run/${name}" "run/${name}.exe"
done

echo ""
echo "=== Results: ${PASS} passed, ${FAIL} failed, ${SKIP} skipped ==="

if [ $FAIL -gt 0 ]; then
    exit 1
fi
