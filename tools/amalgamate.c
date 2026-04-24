/**
 * @file
 * @brief   amalgamate.c -- combines modular lfg-ctest sources into a single header.
 *
 * Usage: amalgamate <manifest> <output> [search_dir...]
 *
 * Trailing search_dir arguments are consulted in order when resolving each
 * manifest entry; the first hit wins. If none are given, the tool falls
 * back to "." (the process's working directory). This lets generated files
 * (e.g. lfg-ctest-version.h in the build dir) be folded into the output
 * alongside the checked-in sources.
 *
 * The manifest lists files in order with section markers:
 *   \@header_begin / \@header_end   -- public header section
 *   \@impl_begin   / \@impl_end     -- implementation section
 *
 * The tool strips internal \#include directives and include guards,
 * deduplicates system \#includes, and wraps everything in the standard
 * single-header pattern (\#ifndef LFG_CTEST_SINGLE_H_ / \#ifdef
 * LFG_CTEST_IMPLEMENTATION).
 *
 * Include-guard detection is deliberately strict: only identifiers ending
 * in "_H_" are treated as guards. That matches this repo's convention and
 * avoids false-matching operational macros like LFG_CTEST_HAS_FLOAT on an
 * \#endif comment.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE 4096
#define MAX_FILES 64
#define MAX_SYS_INCLUDES 128
#define MAX_GUARD 128

/*============================================================================
 *  Seen-includes tracker (deduplication)
 *==========================================================================*/

static char _seen_includes[MAX_SYS_INCLUDES][256];
static int _seen_count = 0;

static int
already_seen(const char *inc)
{
    int i;
    for (i = 0; i < _seen_count; i++)
    {
        if (strcmp(_seen_includes[i], inc) == 0)
        {
            return 1;
        }
    }
    if (_seen_count < MAX_SYS_INCLUDES)
    {
        strncpy(_seen_includes[_seen_count], inc, sizeof(_seen_includes[0]) - 1);
        _seen_includes[_seen_count][sizeof(_seen_includes[0]) - 1] = '\0';
        _seen_count++;
    }
    return 0;
}

/*============================================================================
 *  Line classification
 *==========================================================================*/

/** Skip leading whitespace. */
static const char *
skip_ws(const char *p)
{
    while (*p == ' ' || *p == '\t')
    {
        p++;
    }
    return p;
}

/** True if @p line is an internal \#include (lfg-ctest.h / lfg-ctest-mock.h). */
static int
is_internal_include(const char *line)
{
    const char *p = skip_ws(line);
    if (strncmp(p, "#include", 8) != 0)
    {
        return 0;
    }
    p = skip_ws(p + 8);
    if (*p != '"')
    {
        return 0;
    }
    p++;
    if (strncmp(p, "lfg-ctest", 9) == 0)
    {
        return 1;
    }
    return 0;
}

/** Extract identifier after a preproc keyword. Returns length, writes id into @p out. */
static size_t
extract_ident(const char *after_kw, char *out, size_t out_sz)
{
    const char *id = skip_ws(after_kw);
    const char *e = id;
    while (*e && *e != ' ' && *e != '\t' && *e != '\n' && *e != '\r' && *e != '/')
    {
        e++;
    }
    size_t len = (size_t)(e - id);
    if (len == 0 || len >= out_sz)
    {
        return 0;
    }
    memcpy(out, id, len);
    out[len] = '\0';
    return len;
}

/** True if @p ident ends with "_H_" -- this repo's guard convention. */
static int
ident_is_guard(const char *ident, size_t len)
{
    return (len >= 3 && memcmp(ident + len - 3, "_H_", 3) == 0);
}

/** Check & strip an include-guard line. State held via @p current_guard. */
static int
is_include_guard(const char *line, char *current_guard, size_t guard_sz)
{
    const char *p = skip_ws(line);
    char ident[MAX_GUARD];

    if (strncmp(p, "#ifndef ", 8) == 0 && current_guard[0] == '\0')
    {
        size_t len = extract_ident(p + 8, ident, sizeof(ident));
        if (len && ident_is_guard(ident, len) && len < guard_sz)
        {
            memcpy(current_guard, ident, len + 1);
            return 1;
        }
    }
    else if (strncmp(p, "#define ", 8) == 0 && current_guard[0] != '\0')
    {
        size_t len = extract_ident(p + 8, ident, sizeof(ident));
        if (len && strcmp(ident, current_guard) == 0)
        {
            return 1;
        }
    }
    else if (strncmp(p, "#endif", 6) == 0 && current_guard[0] != '\0')
    {
        /* Closing #endif: its trailing comment references the guard. */
        if (strstr(p, current_guard))
        {
            current_guard[0] = '\0';
            return 1;
        }
    }

    return 0;
}

/** Extract an \#include \<...\> directive; returns 1 if matched. */
static int
is_system_include(const char *line, char *out_inc, size_t out_sz)
{
    const char *p = skip_ws(line);
    if (strncmp(p, "#include", 8) != 0)
    {
        return 0;
    }
    p = skip_ws(p + 8);
    if (*p != '<')
    {
        return 0;
    }

    const char *start = p;
    const char *end = strchr(p, '>');
    if (!end)
    {
        return 0;
    }
    size_t len = (size_t)(end - start + 1);
    if (len >= out_sz)
    {
        len = out_sz - 1;
    }
    memcpy(out_inc, start, len);
    out_inc[len] = '\0';
    return 1;
}

/*============================================================================
 *  File emission -- write one source file to output, filtering lines
 *==========================================================================*/

static int
emit_file(FILE *out, const char **search_dirs, int n_dirs, const char *relpath)
{
    char fullpath[1024];
    FILE *in = NULL;
    int i;

    for (i = 0; i < n_dirs; i++)
    {
        snprintf(fullpath, sizeof(fullpath), "%s/%s", search_dirs[i], relpath);
        in = fopen(fullpath, "r");
        if (in)
        {
            break;
        }
    }
    if (!in)
    {
        fprintf(stderr, "amalgamate: cannot find '%s' in any search dir\n", relpath);
        return -1;
    }

    fprintf(out, "/* --- %s --- */\n", relpath);

    char line[MAX_LINE];
    char current_guard[MAX_GUARD] = {0};

    while (fgets(line, sizeof(line), in))
    {
        if (is_internal_include(line))
        {
            continue;
        }

        if (is_include_guard(line, current_guard, sizeof(current_guard)))
        {
            continue;
        }

        char inc_key[256];
        if (is_system_include(line, inc_key, sizeof(inc_key)))
        {
            if (already_seen(inc_key))
            {
                continue;
            }
        }

        fputs(line, out);
    }

    fprintf(out, "\n");
    fclose(in);
    return 0;
}

/*============================================================================
 *  Manifest parsing
 *==========================================================================*/

enum section
{
    SECTION_NONE,
    SECTION_HEADER,
    SECTION_IMPL,
};

struct manifest
{
    char header_files[MAX_FILES][256];
    int header_count;
    char impl_files[MAX_FILES][256];
    int impl_count;
};

static int
parse_manifest(const char *path, struct manifest *m)
{
    FILE *f = fopen(path, "r");
    if (!f)
    {
        fprintf(stderr, "amalgamate: cannot open manifest '%s'\n", path);
        return -1;
    }

    memset(m, 0, sizeof(*m));
    enum section sec = SECTION_NONE;
    char line[MAX_LINE];

    while (fgets(line, sizeof(line), f))
    {
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
        {
            line[--len] = '\0';
        }

        if (len == 0 || line[0] == '#')
        {
            continue;
        }

        if (strcmp(line, "@header_begin") == 0)
        {
            sec = SECTION_HEADER;
            continue;
        }
        if (strcmp(line, "@header_end") == 0)
        {
            sec = SECTION_NONE;
            continue;
        }
        if (strcmp(line, "@impl_begin") == 0)
        {
            sec = SECTION_IMPL;
            continue;
        }
        if (strcmp(line, "@impl_end") == 0)
        {
            sec = SECTION_NONE;
            continue;
        }

        switch (sec)
        {
        case SECTION_HEADER:
            if (m->header_count < MAX_FILES)
            {
                strncpy(m->header_files[m->header_count], line, sizeof(m->header_files[0]) - 1);
                m->header_count++;
            }
            break;
        case SECTION_IMPL:
            if (m->impl_count < MAX_FILES)
            {
                strncpy(m->impl_files[m->impl_count], line, sizeof(m->impl_files[0]) - 1);
                m->impl_count++;
            }
            break;
        default:
            break;
        }
    }

    fclose(f);
    return 0;
}

/*============================================================================
 *  Main
 *==========================================================================*/

int
main(int argc, char **argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "usage: amalgamate <manifest> <output> [search_dir...]\n");
        return 1;
    }

    const char *manifest_path = argv[1];
    const char *output_path = argv[2];

    /* Search dirs: CLI args after <output>, or "." as a single default. */
    const char *default_dirs[] = {"."};
    const char **search_dirs;
    int n_search_dirs;
    if (argc > 3)
    {
        search_dirs = (const char **)&argv[3];
        n_search_dirs = argc - 3;
    }
    else
    {
        search_dirs = default_dirs;
        n_search_dirs = 1;
    }

    struct manifest m;
    if (parse_manifest(manifest_path, &m) != 0)
    {
        return 1;
    }

    if (m.header_count == 0)
    {
        fprintf(stderr, "amalgamate: no header files in manifest\n");
        return 1;
    }

    FILE *out = fopen(output_path, "w");
    if (!out)
    {
        fprintf(stderr, "amalgamate: cannot open output '%s'\n", output_path);
        return 1;
    }

    fprintf(out,
            "/**\n"
            " * @file\n"
            " * @brief   lfg-ctest -- C99 unit test + mock framework (amalgamated).\n"
            " *\n"
            " * This file is auto-generated. Do not edit.\n"
            " * Define LFG_CTEST_IMPLEMENTATION in exactly one translation unit\n"
            " * before including this header:\n"
            " *\n"
            " *     #define LFG_CTEST_IMPLEMENTATION\n"
            " *     #include \"lfg-ctest.h\"\n"
            " *\n"
            " * Optional build-time defines:\n"
            " *   LFG_CTEST_HAS_FLOAT    -- enable 32-bit float asserts (needs libm).\n"
            " *   LFG_CTEST_HAS_DOUBLE   -- enable 64-bit double asserts (needs libm).\n"
            " *   LFG_CTEST_NO_FUNC      -- disable __func__ reporting.\n"
            " *   LFG_CTEST_SELF_TEST    -- enable expect-failures self-test mode.\n"
            " */\n\n"
            "#ifndef LFG_CTEST_SINGLE_H_\n"
            "#define LFG_CTEST_SINGLE_H_\n\n");

    int i;
    for (i = 0; i < m.header_count; i++)
    {
        if (emit_file(out, search_dirs, n_search_dirs, m.header_files[i]) != 0)
        {
            fclose(out);
            return 1;
        }
    }

    fprintf(out,
            "/*============================================================================\n"
            " *  Implementation\n"
            " *==========================================================================*/\n\n"
            "#ifdef LFG_CTEST_IMPLEMENTATION\n\n");

    for (i = 0; i < m.impl_count; i++)
    {
        if (emit_file(out, search_dirs, n_search_dirs, m.impl_files[i]) != 0)
        {
            fclose(out);
            return 1;
        }
    }

    fprintf(out, "#endif /* LFG_CTEST_IMPLEMENTATION */\n");
    fprintf(out, "#endif /* LFG_CTEST_SINGLE_H_ */\n");

    fclose(out);
    printf("amalgamate: wrote %s\n", output_path);
    return 0;
}
