#!/usr/bin/env bash
#
# migrate-3arg.sh — the mechanical half of the registration-time
# setup/teardown migration (lfg/ctest#35 "Migration and deprecation path").
#
# The pre-body-only API bound setup/teardown at registration time. The revert
# to body-only registration is a two-shaped transform:
#
#   1. lfg_ct_test(NULL, X, NULL)  -> lfg_ct_test(X)     (fixtureless: mechanical)
#      lfg_ct_suite(NULL, X, NULL) -> lfg_ct_suite(X)
#   2. lfg_ct_test(S, X, T)        -> lfg_ct_test(X) + move S()/T() into the
#                                     body                (fixtured: human edit)
#
# This script does shape (1) only — it strips the NULL/NULL fixtureless form in
# place — and REPORTS every shape (2) site it deliberately left alone, so the
# human knows exactly what remains. Shape (2) cannot be scripted safely: where
# S()/T() go in the body (and whether the failure-count / teardown-before-skip
# patterns apply) is a per-call-site judgement. See docs/migration-3arg.md.
#
# Usage:
#   tools/migrate-3arg.sh [--apply] <file-or-dir>...
#
# Default is a dry run: it prints what it would rewrite and what still needs a
# human, and changes nothing. Pass --apply to rewrite matched files in place.
#
# Limitation: the matching is line-oriented. A single 3-arg registration split
# across multiple physical lines is neither stripped nor reported — reflow such
# call sites onto one line (clang-format does this) before running the codemod.
#
# Exit status: 0 on success, 2 on usage error.

set -euo pipefail

# A fixtureless 3-arg registration: lfg_ct_test / lfg_ct_suite with a NULL
# setup, a single identifier body, and a NULL teardown. Whitespace-tolerant.
readonly MECH_MATCH='lfg_ct_(test|suite)[[:space:]]*\([[:space:]]*NULL[[:space:]]*,[[:space:]]*[A-Za-z_][A-Za-z0-9_]*[[:space:]]*,[[:space:]]*NULL[[:space:]]*\)'
readonly MECH_SUBST='s/(lfg_ct_(test|suite))[[:space:]]*\([[:space:]]*NULL[[:space:]]*,[[:space:]]*([A-Za-z_][A-Za-z0-9_]*)[[:space:]]*,[[:space:]]*NULL[[:space:]]*\)/\1(\3)/g'

# A fixtured 3-arg registration: a call with two commas between the parens that
# is NOT the pure NULL/NULL form. Reported, never rewritten.
readonly FIXTURED_MATCH='lfg_ct_(test|suite)[[:space:]]*\([^()]*,[^()]*,[^()]*\)'


usage()
{
    printf 'usage: %s [--apply] <file-or-dir>...\n' "${0##*/}" >&2
    return 2
}


# Echo every *.c / *.h file under the given paths (files pass through as-is).
collect_files()
{
    local path
    for path in "$@"
    do
        if [ -d "$path" ]
        then
            find "$path" -type f \( -name '*.c' -o -name '*.h' \) | sort
        elif [ -f "$path" ]
        then
            printf '%s\n' "$path"
        else
            printf 'skip: not a file or directory: %s\n' "$path" >&2
        fi
    done
}


# Rewrite one file's fixtureless sites in place (only when apply=1) and report.
process_file()
{
    local file="$1" apply="$2"
    local mech residue transformed

    # Count occurrences, not matching lines: the /g subst rewrites every site
    # on a line, so a line with two registrations must count as two.
    mech="$(grep -oE "$MECH_MATCH" "$file" | grep -c '' || true)"

    # Residual fixtured sites are computed against the POST-transform text, so
    # the "needs human" list never counts a NULL/NULL site the strip removes.
    # The subst is per-line, so line numbers stay aligned with the original.
    transformed="$(sed -E "$MECH_SUBST" "$file")"
    residue="$(printf '%s\n' "$transformed" | grep -nE "$FIXTURED_MATCH" || true)"

    if [ "$mech" -gt 0 ]
    then
        if [ "$apply" -eq 1 ]
        then
            sed -E -i "$MECH_SUBST" "$file"
            printf 'rewrote %d fixtureless site(s): %s\n' "$mech" "$file"
        else
            printf 'would rewrite %d fixtureless site(s): %s\n' "$mech" "$file"
        fi
    fi

    if [ -n "$residue" ]
    then
        local count
        count="$(printf '%s\n' "$residue" | grep -c '' || true)"
        printf 'NEEDS HUMAN — %d fixtured site(s) in %s:\n' "$count" "$file"
        printf '%s\n' "$residue" | sed 's/^/    /'
    fi
}


main()
{
    local apply=0
    local -a paths=()

    while [ "$#" -gt 0 ]
    do
        case "$1" in
            --apply) apply=1; shift ;;
            --) shift; break ;;
            -*) usage ;;
            *) paths+=("$1"); shift ;;
        esac
    done
    # Anything after a bare -- is a path too.
    while [ "$#" -gt 0 ]
    do
        paths+=("$1"); shift
    done

    [ "${#paths[@]}" -eq 0 ] && usage

    local file
    while IFS= read -r file
    do
        process_file "$file" "$apply"
    done < <(collect_files "${paths[@]}")

    if [ "$apply" -eq 0 ]
    then
        printf '\n(dry run — nothing changed. Re-run with --apply to rewrite.)\n'
    fi
}

main "$@"
