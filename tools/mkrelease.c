/**
 * @file
 * @brief       mkrelease -- interactively stamp a release-v<M>.<m>.<p> tag
 *                           that matches the current `git describe` output.
 *
 * Usage:
 *     mkrelease [source_dir]
 *
 * Flow:
 *   1. Run `git -C <src> describe --match 'v*'` -> parse M.m.p.
 *   2. Show the current state and proposed tag; prompt for confirmation.
 *   3. On 'y', exec `git -C <src> tag -a <tag>` so $EDITOR handles the
 *      annotation message (same UX as running `git tag -a` directly).
 *
 * Bails with a diagnostic on:
 *   - `git describe` failure (no matching tag or not a repo)
 *   - proposed tag already exists
 *   - declined at the prompt
 *
 * The existence of this tool is pure convenience: human arithmetic on
 * `git describe` output keeps drifting because the distance number
 * advances with every commit, and a hand-typed `release-vX.Y.Z` that
 * lags by even one patch silently ships a stale version string.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_CMD  1024
#define MAX_LINE 256
#define MAX_TAG  64

/** Run a shell command and read the first line of stdout into @p out.
 *  Returns 0 on success (non-empty line captured), -1 otherwise. */
static int
capture_line(const char *cmd, char *out, size_t out_sz)
{
    FILE *fp;
    char *nl;

    fp = popen(cmd, "r");
    if (!fp)
    {
        return -1;
    }
    if (!fgets(out, out_sz, fp))
    {
        pclose(fp);
        out[0] = '\0';
        return -1;
    }
    pclose(fp);
    nl = strchr(out, '\n');
    if (nl)
    {
        *nl = '\0';
    }
    return out[0] ? 0 : -1;
}

/** Read one line from stdin and return 1 iff it starts with 'y' or 'Y'. */
static int
prompt_yes(void)
{
    char buf[16];

    if (!fgets(buf, sizeof(buf), stdin))
    {
        return 0;
    }
    return buf[0] == 'y' || buf[0] == 'Y';
}

int
main(int argc, char **argv)
{
    const char *source_dir = (argc >= 2) ? argv[1] : ".";
    char cmd[MAX_CMD];
    char desc[MAX_LINE];
    char head[MAX_LINE];
    char sha_of_tag[MAX_LINE];
    char tag[MAX_TAG];
    int major = 0, minor = 0, patch = 0;
    const char *p;
    int n;

    /* ── Resolve current version from base v* tag ─────────────── */
    snprintf(cmd, sizeof(cmd),
             "git -C \"%s\" describe --match 'v*' 2>/dev/null",
             source_dir);
    if (capture_line(cmd, desc, sizeof(desc)) != 0)
    {
        fprintf(stderr,
                "mkrelease: `git describe --match 'v*'` produced nothing.\n"
                "           Need an annotated base tag (e.g. v0.1) first.\n");
        return 1;
    }

    /* Strip the leading 'v' then parse M.m[-p]. Accepts:
     *   v0.1-48-gXXX  -> 0, 1, 48
     *   v0.1          -> 0, 1, 0   (exact-match, distance = 0) */
    p = desc;
    if (*p == 'v')
    {
        p++;
    }
    n = sscanf(p, "%d.%d-%d", &major, &minor, &patch);
    if (n == 2)
    {
        patch = 0;
    }
    else if (n != 3)
    {
        fprintf(stderr, "mkrelease: can't parse describe output '%s'\n", desc);
        return 1;
    }

    snprintf(tag, sizeof(tag), "release-v%d.%d.%d", major, minor, patch);

    /* ── HEAD SHA for display ─────────────────────────────────── */
    snprintf(cmd, sizeof(cmd),
             "git -C \"%s\" rev-parse --short HEAD 2>/dev/null",
             source_dir);
    if (capture_line(cmd, head, sizeof(head)) != 0)
    {
        strcpy(head, "?");
    }

    /* ── Refuse to clobber an existing tag ────────────────────── */
    snprintf(cmd, sizeof(cmd),
             "git -C \"%s\" rev-parse --verify --quiet \"refs/tags/%s\" 2>/dev/null",
             source_dir, tag);
    if (capture_line(cmd, sha_of_tag, sizeof(sha_of_tag)) == 0)
    {
        fprintf(stderr,
                "mkrelease: tag '%s' already exists (at %.7s). "
                "Delete it first or bump the base tag.\n",
                tag, sha_of_tag);
        return 1;
    }

    /* ── Confirm with the user ────────────────────────────────── */
    printf("\n");
    printf("  git describe: %s\n", desc);
    printf("  HEAD:         %s\n", head);
    printf("  proposed tag: %s\n", tag);
    printf("\n");
    printf("Create annotated tag '%s' at HEAD? [y/N]: ", tag);
    fflush(stdout);
    if (!prompt_yes())
    {
        fprintf(stderr, "mkrelease: aborted.\n");
        return 1;
    }

    /* ── Hand off to `git tag -a <tag>` so $EDITOR drives the message ── */
    {
        char *git_argv[] = {
            "git", "-C", (char *)source_dir,
            "tag", "-a", tag, NULL
        };
        execvp("git", git_argv);
        perror("mkrelease: execvp git");
        return 1;
    }
}
