/**
 * @file
 * @brief       mkversion -- emit a C header with version macros derived from `git describe`.
 *
 * Usage:
 *     mkversion <prefix> [source_dir]
 *
 *   <prefix>       macro/guard prefix (e.g. "LFG_CTEST"). Uppercased as-is.
 *   [source_dir]   directory whose git state to describe (passed to `git -C`).
 *                  Defaults to the current working directory.
 *
 * Output (stdout) is a self-contained header:
 *
 *     #ifndef <PREFIX>_VERSION_H_
 *     #define <PREFIX>_VERSION_H_
 *     #define <PREFIX>_VERSION_MAJOR <n>
 *     #define <PREFIX>_VERSION_MINOR <n>
 *     #define <PREFIX>_VERSION_PATCH <n>
 *     #define <PREFIX>_VERSION      "M.m.p"
 *     #define <PREFIX>_VERSION_FULL "M.m.p[+<sha>]"
 *     #endif
 *
 * Two-tier tag lookup:
 *
 *   1. `git -C <src> describe --match 'release-v*' --exact-match`
 *      When HEAD is exactly on a release tag like `release-v0.1.42`,
 *      emit a clean `0.1.42` with no build metadata. Releases are
 *      immutable markers, so the `+<sha>` trailer is noise.
 *
 *   2. `git -C <src> describe --match 'v*'`
 *      Everyday builds ride the base tag (e.g. `v0.1`). `git describe`
 *      gives `v<M>.<m>-<distance>-g<sha>`; we emit `M.m.<distance>+<sha>`.
 *
 * Fallback when neither lookup succeeds: emits 0.0.0. This keeps vendored
 * tarballs compiling without a .git tree.
 *
 * The tool is intentionally prefix-agnostic so it can be reused by any
 * project that follows the v<major>.<minor> tagging convention. Keep it
 * self-contained; no project-specific logic.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CMD  1024
#define MAX_LINE 128
#define MAX_HASH 32

/* Release tag prefix (length must match the literal below). */
#define RELEASE_PREFIX     "release-v"
#define RELEASE_PREFIX_LEN 9

int
main(int argc, char **argv)
{
    const char *prefix;
    const char *source_dir;
    char cmd[MAX_CMD];
    char buf[MAX_LINE];
    FILE *fp;
    int major = 0, minor = 0, patch = 0;
    char hash[MAX_HASH] = "";
    char *p;
    int parsed = 0;

    if (argc < 2 || argc > 3)
    {
        fprintf(stderr, "usage: mkversion <prefix> [source_dir]\n");
        return 1;
    }
    prefix = argv[1];
    source_dir = (argc == 3) ? argv[2] : ".";

    /* ── Tier 1: exact release tag ─────────────────────────────
     * --exact-match succeeds only when HEAD is exactly on a
     * `release-v*` tag. When it does, we emit a clean version
     * with no +<sha> trailer. */
    snprintf(cmd, sizeof(cmd),
             "git -C \"%s\" describe --match '" RELEASE_PREFIX "*' --exact-match 2>/dev/null",
             source_dir);
    fp = popen(cmd, "r");
    if (fp && fgets(buf, sizeof(buf), fp))
    {
        p = strchr(buf, '\n');
        if (p)
        {
            *p = '\0';
        }
        if (strncmp(buf, RELEASE_PREFIX, RELEASE_PREFIX_LEN) == 0
            && sscanf(buf + RELEASE_PREFIX_LEN, "%d.%d.%d", &major, &minor, &patch) == 3)
        {
            parsed = 1;
            /* hash stays empty → VERSION_FULL omits the trailer. */
        }
    }
    if (fp)
    {
        pclose(fp);
    }

    /* ── Tier 2: base `v<M>.<m>` tag with distance ─────────────
     * Picks up everyday in-development builds. */
    if (!parsed)
    {
        snprintf(cmd, sizeof(cmd),
                 "git -C \"%s\" describe --match 'v*' 2>/dev/null",
                 source_dir);
        fp = popen(cmd, "r");
        if (fp && fgets(buf, sizeof(buf), fp))
        {
            p = strchr(buf, '\n');
            if (p)
            {
                *p = '\0';
            }
            p = buf;
            if (*p == 'v')
            {
                p++;
            }
            /* Try full form with commit distance + hash, then fall back
             * to a bare `v<major>.<minor>` tag on an exact match. */
            if (sscanf(p, "%d.%d-%d-g%31s", &major, &minor, &patch, hash) < 2)
            {
                sscanf(p, "%d.%d", &major, &minor);
            }
        }
        if (fp)
        {
            pclose(fp);
        }
    }

    printf("#ifndef %s_VERSION_H_\n", prefix);
    printf("#define %s_VERSION_H_\n", prefix);
    printf("#define %s_VERSION_MAJOR %d\n", prefix, major);
    printf("#define %s_VERSION_MINOR %d\n", prefix, minor);
    printf("#define %s_VERSION_PATCH %d\n", prefix, patch);
    printf("#define %s_VERSION      \"%d.%d.%d\"\n", prefix, major, minor, patch);
    if (hash[0])
    {
        printf("#define %s_VERSION_FULL \"%d.%d.%d+%s\"\n",
               prefix, major, minor, patch, hash);
    }
    else
    {
        printf("#define %s_VERSION_FULL \"%d.%d.%d\"\n",
               prefix, major, minor, patch);
    }
    printf("#endif /* %s_VERSION_H_ */\n", prefix);

    return 0;
}
