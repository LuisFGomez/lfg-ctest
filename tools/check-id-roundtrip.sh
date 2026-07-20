#!/usr/bin/env bash
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
# Usage: check-id-roundtrip.sh <test-binary>

set -euo pipefail

BIN="${1:?usage: check-id-roundtrip.sh <test-binary>}"

if [ ! -x "$BIN" ]
then
    echo "check-id-roundtrip: not executable: $BIN" >&2
    exit 1
fi

# --list writes ids to stdout; the banner the runner prints around them goes
# there too, so select the id lines by their two-separator shape rather than
# assuming the listing is the whole stream.
mapfile -t IDS < <("$BIN" --list | grep -E '^[^ ]+\.c::[^ :]+::[^ :]+$')

if [ "${#IDS[@]}" -eq 0 ]
then
    echo "check-id-roundtrip: $BIN listed no fully-qualified ids" >&2
    exit 1
fi

FAILED=0

for id in "${IDS[@]}"
do
    # A CR surviving from the listing would land inside the glob and match
    # nothing -- catch it as itself rather than as a confusing count of 0.
    case "$id" in
        *$'\r'*)
            echo "check-id-roundtrip: id carries a trailing CR: $(printf '%q' "$id")" >&2
            FAILED=1
            continue
            ;;
    esac

    # A self-test binary may exit non-zero by design (it verifies failure
    # detection); the summary line is the signal, not the exit code.
    count=$("$BIN" --filter "$id" 2>/dev/null | sed -n 's/.*Executed [0-9]* assertions in \([0-9]*\) tests.*/\1/p' || true)

    if [ "$count" != "1" ]
    then
        echo "check-id-roundtrip: '$id' selected ${count:-<no summary>} tests, expected 1" >&2
        FAILED=1
    fi
done

if [ "$FAILED" -ne 0 ]
then
    exit 1
fi

echo "check-id-roundtrip: ${#IDS[@]} ids each selected exactly 1 test"
