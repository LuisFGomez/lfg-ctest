#!/bin/sh
#
# check-id-roundtrip.sh — assert that `--list` output composes back into
# `--filter` (lfg/ctest#55 "No way to address a single test").
#
# The in-process tests in test-unified.c cover the matching rule; they cannot
# cover this, because the property under test is about the *bytes* --list puts
# on stdout. A trailing CR, a decorative header line, or a shell-special
# character in an id would all leave the matcher correct and the pipeline
# broken. So this runs the pipeline for real:
#
#   ./bin --list | grep <pat> | xargs -I{} ./bin --filter '{}'
#
# and asserts each listed id selects exactly one entry.
#
# Strictly POSIX sh: CMake gates this on if(UNIX), which includes macOS, and
# stock macOS ships bash 3.2 -- so no `mapfile`, no process substitution, no
# arrays, no `$'\r'`, no `printf %q`.
#
# Usage: check-id-roundtrip.sh <test-binary>

set -eu

BIN="${1:?usage: check-id-roundtrip.sh <test-binary>}"

if [ ! -x "$BIN" ]
then
    echo "check-id-roundtrip: not executable: $BIN" >&2
    exit 1
fi

LIST=$(mktemp) || exit 1

# Every run below persists a rerun state file. Point it at a temporary so the
# check does not drop a stray `.lfg-ctest-last` in whatever directory CTest
# happened to invoke it from. `.tmp` is the sibling _state_write renames from.
STATE=$(mktemp) || exit 1
trap 'rm -f "$LIST" "$STATE" "$STATE.tmp"' EXIT INT TERM

CR=$(printf '\r')

# --list writes ids to stdout; the banner the runner prints around them goes
# there too, so select the id lines by their two-separator shape rather than
# assuming the listing is the whole stream.
"$BIN" --list --state-file "$STATE" | grep -E '^[^ ]+\.c::[^ :]+::[^ :]+$' > "$LIST" || true

TOTAL=$(wc -l < "$LIST" | tr -d ' ')

if [ "$TOTAL" -eq 0 ]
then
    echo "check-id-roundtrip: $BIN listed no fully-qualified ids" >&2
    exit 1
fi

FAILED=0

while IFS= read -r id
do
    # A CR surviving from the listing would land inside the glob and match
    # nothing -- catch it as itself rather than as a confusing count of 0.
    case "$id" in
        *"$CR"*)
            echo "check-id-roundtrip: id carries a trailing CR: [$id]" >&2
            FAILED=1
            continue
            ;;
    esac

    # A self-test binary may exit non-zero by design (it verifies failure
    # detection); the summary line is the signal, not the exit code.
    count=$("$BIN" --filter "$id" --state-file "$STATE" 2>/dev/null | sed -n 's/.*Executed [0-9]* assertions in \([0-9]*\) tests.*/\1/p' || true)

    if [ "$count" != "1" ]
    then
        echo "check-id-roundtrip: '$id' selected ${count:-<no summary>} tests, expected 1" >&2
        FAILED=1
    fi
done < "$LIST"

if [ "$FAILED" -ne 0 ]
then
    exit 1
fi

echo "check-id-roundtrip: $TOTAL ids each selected exactly 1 test"
