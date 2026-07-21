#!/bin/sh
#
# check-rerun-failed.sh — assert the --rerun-failed state file works on the
# real bytes, in a real process (lfg/ctest#56).
#
# The in-process tests in test-unified.c cover parsing, selection, seed
# precedence and the narrowing cycle. They cannot cover the properties that
# are only true of a whole process: that the file is written at all from a
# normal exit, that it survives a fully-buffered (redirected) stdout, and
# that --list leaves the previous run's file untouched rather than
# truncating it. A user who lists tests and then reruns their failures must
# still get their failures.
#
# Strictly POSIX sh, for the same reason as check-id-roundtrip.sh: CMake
# gates this on if(UNIX), which includes macOS, and stock macOS ships bash
# 3.2 -- so no arrays, no process substitution, no `local`.
#
# Usage: check-rerun-failed.sh <test-binary>

set -eu

BIN="${1:?usage: check-rerun-failed.sh <test-binary>}"

if [ ! -x "$BIN" ]
then
    echo "check-rerun-failed: not executable: $BIN" >&2
    exit 1
fi

WORK=$(mktemp -d) || exit 1
trap 'rm -rf "$WORK"' EXIT INT TERM

STATE="$WORK/state"
OUT="$WORK/out"
FAILED=0

fail()
{
    echo "check-rerun-failed: $1" >&2
    FAILED=1
}

# ---------------------------------------------------------------------------
# A normal run persists the state file, even with stdout a regular file.
# ---------------------------------------------------------------------------

"$BIN" --state-file "$STATE" > "$OUT" 2>&1 || true

if [ ! -f "$STATE" ]
then
    fail "a normal run wrote no state file"
elif ! head -n 1 "$STATE" | grep -q '^lfg-ctest-state 1$'
then
    fail "state file is missing its format marker: $(head -n 1 "$STATE")"
elif ! grep -q '^seed [0-9][0-9]*$' "$STATE"
then
    fail "state file carries no seed record"
fi

# The persisted seed must be the one the run announced -- a replay that
# restores a different seed is not a reproduction.
BANNER_SEED=$(sed -n 's/^\*\*\* random seed is \([0-9][0-9]*\).*/\1/p' "$OUT" | head -n 1)
STATE_SEED=$(sed -n 's/^seed \([0-9][0-9]*\)$/\1/p' "$STATE" | head -n 1)

if [ -z "$BANNER_SEED" ] || [ "$BANNER_SEED" != "$STATE_SEED" ]
then
    fail "persisted seed [$STATE_SEED] does not match the announced seed [$BANNER_SEED]"
fi

# ---------------------------------------------------------------------------
# --list must not write or truncate the state file.
# ---------------------------------------------------------------------------

ID=$("$BIN" --list --state-file "$STATE" | grep -E '^[^ ]+\.c::[^ :]+::[^ :]+$' | head -n 1 || true)

if [ -z "$ID" ]
then
    fail "$BIN listed no fully-qualified ids"
fi

printf 'lfg-ctest-state 1\nseed 4242\nfail %s\n' "$ID" > "$STATE"
BEFORE=$(cat "$STATE")

"$BIN" --list --state-file "$STATE" > /dev/null 2>&1 || true

if [ "$(cat "$STATE")" != "$BEFORE" ]
then
    fail "--list clobbered the state file"
fi

# ---------------------------------------------------------------------------
# --rerun-failed replays exactly the persisted set, under the persisted seed.
# ---------------------------------------------------------------------------

"$BIN" --rerun-failed --state-file "$STATE" > "$OUT" 2>&1 || true

COUNT=$(sed -n 's/.*Executed [0-9]* assertions in \([0-9]*\) tests.*/\1/p' "$OUT" | head -n 1)

if [ "${COUNT:-}" != "1" ]
then
    fail "--rerun-failed on one key ran ${COUNT:-<no summary>} tests, expected 1"
fi

if ! grep -q '^\*\*\* random seed is 4242' "$OUT"
then
    fail "--rerun-failed did not restore the persisted seed"
fi

# An explicit --seed outranks the persisted one.
printf 'lfg-ctest-state 1\nseed 4242\nfail %s\n' "$ID" > "$STATE"
"$BIN" --rerun-failed --seed 31337 --state-file "$STATE" > "$OUT" 2>&1 || true

if ! grep -q '^\*\*\* random seed is 31337' "$OUT"
then
    fail "an explicit --seed did not override the persisted seed"
fi

# The rerun rewrote the file with its own (empty) failure set, so repeating
# the flag converges instead of replaying the original set forever.
if grep -q '^fail ' "$STATE"
then
    fail "a green --rerun-failed run left stale keys in the state file"
fi

# ---------------------------------------------------------------------------
# Error paths: never a silent full-suite run.
# ---------------------------------------------------------------------------

check_error_exit()
{
    # $1 label, $2 state-file contents ("-" for "no file at all")
    if [ "$2" = "-" ]
    then
        rm -f "$STATE"
    else
        printf '%s' "$2" > "$STATE"
    fi

    if "$BIN" --rerun-failed --state-file "$STATE" > "$OUT" 2>&1
    then
        fail "$1: exited 0"
        return
    fi
    if ! grep -q 'rerun-failed' "$OUT"
    then
        fail "$1: no diagnostic mentioning --rerun-failed"
    fi
    if grep -q 'Executed [0-9]* assertions in [1-9]' "$OUT"
    then
        fail "$1: fell through to running tests"
    fi
}

check_error_exit "missing state file" "-"
check_error_exit "malformed state file" "not a state file
"
check_error_exit "state file with no seed" "lfg-ctest-state 1
fail nope.c::nope::nope
"
check_error_exit "wholly unresolvable state file" "lfg-ctest-state 1
seed 1
fail no-such-file.c::no_such_suite::no_such_test
"

if [ "$FAILED" -ne 0 ]
then
    exit 1
fi

echo "check-rerun-failed: state file round trip, --list safety and error paths OK"
