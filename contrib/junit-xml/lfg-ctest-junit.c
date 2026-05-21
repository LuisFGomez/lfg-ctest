/**
 * @file
 * @brief       Opt-in JUnit-XML report emitter for lfg-ctest.
 *
 * Internal data flow:
 *   - @ref lfg_ct_junit_consume_args (or @ref lfg_ct_junit_set_output)
 *     installs a file-static @ref lfg_ct_reporter_t whose @c on_record
 *     callback appends each test's record to a dynamic case array.
 *   - @c on_run_complete (fired by the runner at the tail of
 *     @ref lfg_ct_print_summary) renders the accumulated cases into
 *     a Jenkins-flavored JUnit-XML document and writes it to the
 *     configured path.
 *   - A write failure emits a stderr warning but does @b not change
 *     the run's exit code; that contract belongs to the runner.
 *
 * No public state, no globals exposed beyond the two functions in
 * @c lfg-ctest-junit.h.
 */

/*============================================================================
 *  Includes
 *==========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lfg-ctest.h"
#include "lfg-ctest-junit.h"

/*============================================================================
 *  Defines/Typedefs
 *==========================================================================*/

#define JUNIT_INITIAL_CAP 64
#define JUNIT_NAME_MAX 192
#define JUNIT_MSG_MAX 512

/** Growable byte buffer used by the renderer. The @c data pointer
 *  is always NUL-terminated when non-NULL so callers can read it as
 *  a C string. */
typedef struct
{
    char *data;
    size_t len;
    size_t cap;
    int oom;
} _xml_buf_t;

/** One captured test outcome, copied off the runner's borrowed
 *  pointers at @c on_record time (the runner overwrites its
 *  @c _current_failure_msg at the next test's entry). */
typedef struct
{
    char classname[JUNIT_NAME_MAX];
    char name[JUNIT_NAME_MAX];
    double time_sec;
    lfg_ct_outcome_t outcome;
    char message[JUNIT_MSG_MAX];
    int has_message;
} _junit_case_t;

/*============================================================================
 *  Variables
 *==========================================================================*/

static _junit_case_t *_cases = NULL;
static size_t _cases_count = 0;
static size_t _cases_cap = 0;
static const char *_output_path = NULL;
static char _suite_name[JUNIT_NAME_MAX] = "lfg-ctest";

/* Forward declarations so the reporter struct (file-static at the
 * bottom of this file) can reference them at init time. */
static void _on_record(const lfg_ct_record_t *rec, void *userdata);
static void _on_run_complete(void *userdata);

/* Reporter struct -- file-static so its address stays valid for
 * the lifetime of the test binary, which is what
 * @ref lfg_ct_set_reporter borrows it for. */
static const lfg_ct_reporter_t _reporter = {_on_record, _on_run_complete, NULL, NULL};

/*============================================================================
 *  Private Function Prototypes
 *==========================================================================*/

static void _copy_truncate(char *dst, size_t dst_size, const char *src);
static int _grow_cases(void);
static int _emit_to_file(const char *path);
static void _record_reset(void);

static void _xml_buf_init(_xml_buf_t *b);
static void _xml_buf_free(_xml_buf_t *b);
static void _xml_buf_reserve(_xml_buf_t *b, size_t extra);
static void _xml_buf_append(_xml_buf_t *b, const char *s);
static void _xml_buf_append_len(_xml_buf_t *b, const char *s, size_t n);
static void _xml_buf_append_attr(_xml_buf_t *b, const char *s);
static void _xml_buf_append_text(_xml_buf_t *b, const char *s);
static void _xml_buf_append_double(_xml_buf_t *b, double v);
static void _xml_buf_append_size(_xml_buf_t *b, size_t v);

static char *_render(size_t *out_len);
static void _render_case(_xml_buf_t *b, const _junit_case_t *c, const char *fallback_classname);

/*============================================================================
 *  Public API
 *==========================================================================*/

int
lfg_ct_junit_consume_args(int argc, char *argv[])
{
    int i;
    int j;

    if (argc <= 0 || NULL == argv)
    {
        return argc;
    }

    for (i = 1; i < argc; i++)
    {
        if (NULL == argv[i])
        {
            continue;
        }
        if (0 != strcmp(argv[i], "--output-junit"))
        {
            continue;
        }
        if (i + 1 >= argc)
        {
            fprintf(stderr, "%s: --output-junit requires an argument\r\n",
                    argv[0] ? argv[0] : "test");
            return -1;
        }
        {
            const char *path = argv[i + 1];
            const char *suite_name = argv[0] ? argv[0] : "lfg-ctest";
            const char *slash = strrchr(suite_name, '/');
            if (slash)
            {
                suite_name = slash + 1;
            }
            lfg_ct_junit_set_output(path, suite_name);
        }
        /* Peel out the two consumed entries by shifting the rest
         * of argv left by two. */
        for (j = i; j + 2 < argc; j++)
        {
            argv[j] = argv[j + 2];
        }
        argv[argc - 2] = NULL;
        argv[argc - 1] = NULL;
        argc -= 2;
        /* Only one --output-junit flag is meaningful (last write
         * would otherwise win silently). Stop after consuming the
         * first occurrence. */
        break;
    }
    return argc;
}

void
lfg_ct_junit_set_output(const char *path, const char *suite_name)
{
    _output_path = path;
    if (suite_name && suite_name[0])
    {
        _copy_truncate(_suite_name, sizeof(_suite_name), suite_name);
    }
    else
    {
        _copy_truncate(_suite_name, sizeof(_suite_name), "lfg-ctest");
    }
    if (NULL != _output_path)
    {
        lfg_ct_set_reporter(&_reporter);
    }
    else
    {
        lfg_ct_set_reporter(NULL);
        _record_reset();
    }
}

/*============================================================================
 *  Reporter callbacks (file-static; bound via _reporter struct)
 *==========================================================================*/

static void
_on_record(const lfg_ct_record_t *rec, void *userdata)
{
    _junit_case_t *c;

    (void)userdata;
    if (NULL == rec || NULL == rec->test_name || '\0' == rec->test_name[0])
    {
        return;
    }
    if (_cases_count >= _cases_cap)
    {
        if (0 != _grow_cases())
        {
            return;
        }
    }
    c = &_cases[_cases_count++];

    if (rec->suite_name && rec->suite_name[0])
    {
        _copy_truncate(c->classname, sizeof(c->classname), rec->suite_name);
    }
    else
    {
        c->classname[0] = '\0';
    }
    _copy_truncate(c->name, sizeof(c->name), rec->test_name);
    c->time_sec = (rec->time_sec < 0.0) ? 0.0 : rec->time_sec;
    c->outcome = rec->outcome;
    if (rec->message && rec->message[0])
    {
        _copy_truncate(c->message, sizeof(c->message), rec->message);
        c->has_message = 1;
    }
    else
    {
        c->message[0] = '\0';
        c->has_message = 0;
    }
}

static void
_on_run_complete(void *userdata)
{
    (void)userdata;
    if (NULL == _output_path)
    {
        return;
    }
    /* Best-effort flush. Failures warn to stderr inside _emit_to_file
     * and do not propagate into the runner's exit code -- the JUnit
     * report is augmentation, not gating signal. */
    (void)_emit_to_file(_output_path);
    /* After flush, drop the buffered cases so a re-flush (e.g. a
     * test binary that runs multiple sessions) starts clean. */
    _record_reset();
}

/*============================================================================
 *  Private Functions
 *==========================================================================*/

static void
_copy_truncate(char *dst, size_t dst_size, const char *src)
{
    size_t n;

    if (0 == dst_size)
    {
        return;
    }
    if (NULL == src)
    {
        dst[0] = '\0';
        return;
    }
    n = strlen(src);
    if (n >= dst_size)
    {
        n = dst_size - 1;
    }
    memcpy(dst, src, n);
    dst[n] = '\0';
}

static int
_grow_cases(void)
{
    size_t new_cap;
    _junit_case_t *p;

    new_cap = (0 == _cases_cap) ? JUNIT_INITIAL_CAP : _cases_cap * 2;
    p = (_junit_case_t *)realloc(_cases, new_cap * sizeof(_junit_case_t));
    if (NULL == p)
    {
        fprintf(stderr, "lfg-ctest-junit: realloc(%zu cases) failed\r\n", new_cap);
        return -1;
    }
    _cases = p;
    _cases_cap = new_cap;
    return 0;
}

static void
_record_reset(void)
{
    free(_cases);
    _cases = NULL;
    _cases_count = 0;
    _cases_cap = 0;
}

static int
_emit_to_file(const char *path)
{
    char *xml;
    size_t xml_len;
    FILE *fp;
    size_t written;

    xml = _render(&xml_len);
    if (NULL == xml)
    {
        fprintf(stderr, "lfg-ctest-junit: failed to render report (out of memory)\r\n");
        return -1;
    }

    fp = fopen(path, "wb");
    if (NULL == fp)
    {
        fprintf(stderr, "lfg-ctest-junit: failed to open '%s' for writing\r\n", path);
        free(xml);
        return -1;
    }

    written = fwrite(xml, 1, xml_len, fp);
    fclose(fp);
    free(xml);

    if (written != xml_len)
    {
        fprintf(stderr, "lfg-ctest-junit: short write to '%s'\r\n", path);
        return -1;
    }
    return 0;
}

static char *
_render(size_t *out_len)
{
    _xml_buf_t b;
    size_t i;
    size_t tests;
    size_t failures;
    size_t errors;
    size_t skipped;
    double total_time;

    _xml_buf_init(&b);

    tests = _cases_count;
    failures = 0;
    errors = 0;
    skipped = 0;
    total_time = 0.0;
    for (i = 0; i < _cases_count; i++)
    {
        switch (_cases[i].outcome)
        {
        case LFG_CT_FAILED:
            failures++;
            break;
        case LFG_CT_SKIPPED:
        case LFG_CT_XFAIL:
        case LFG_CT_XPASS:
            skipped++;
            break;
        default:
            break;
        }
        total_time += _cases[i].time_sec;
    }

    _xml_buf_append(&b, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    _xml_buf_append(&b, "<testsuites tests=\"");
    _xml_buf_append_size(&b, tests);
    _xml_buf_append(&b, "\" failures=\"");
    _xml_buf_append_size(&b, failures);
    _xml_buf_append(&b, "\" errors=\"");
    _xml_buf_append_size(&b, errors);
    _xml_buf_append(&b, "\" skipped=\"");
    _xml_buf_append_size(&b, skipped);
    _xml_buf_append(&b, "\" time=\"");
    _xml_buf_append_double(&b, total_time);
    _xml_buf_append(&b, "\">\n");

    _xml_buf_append(&b, "  <testsuite name=\"");
    _xml_buf_append_attr(&b, _suite_name);
    _xml_buf_append(&b, "\" tests=\"");
    _xml_buf_append_size(&b, tests);
    _xml_buf_append(&b, "\" failures=\"");
    _xml_buf_append_size(&b, failures);
    _xml_buf_append(&b, "\" errors=\"");
    _xml_buf_append_size(&b, errors);
    _xml_buf_append(&b, "\" skipped=\"");
    _xml_buf_append_size(&b, skipped);
    _xml_buf_append(&b, "\" time=\"");
    _xml_buf_append_double(&b, total_time);
    _xml_buf_append(&b, "\">\n");

    for (i = 0; i < _cases_count; i++)
    {
        _render_case(&b, &_cases[i], _suite_name);
    }

    _xml_buf_append(&b, "  </testsuite>\n");
    _xml_buf_append(&b, "</testsuites>\n");

    if (b.oom)
    {
        _xml_buf_free(&b);
        if (out_len)
        {
            *out_len = 0;
        }
        return NULL;
    }
    if (out_len)
    {
        *out_len = b.len;
    }
    return b.data;
}

static void
_xml_buf_init(_xml_buf_t *b)
{
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
    b->oom = 0;
}

static void
_xml_buf_free(_xml_buf_t *b)
{
    free(b->data);
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}

static void
_xml_buf_reserve(_xml_buf_t *b, size_t extra)
{
    size_t need;
    size_t new_cap;
    char *p;

    if (b->oom)
    {
        return;
    }
    need = b->len + extra + 1;
    if (need <= b->cap)
    {
        return;
    }
    new_cap = (0 == b->cap) ? 512 : b->cap;
    while (new_cap < need)
    {
        new_cap *= 2;
    }
    p = (char *)realloc(b->data, new_cap);
    if (NULL == p)
    {
        b->oom = 1;
        return;
    }
    b->data = p;
    b->cap = new_cap;
}

static void
_xml_buf_append_len(_xml_buf_t *b, const char *s, size_t n)
{
    if (b->oom)
    {
        return;
    }
    _xml_buf_reserve(b, n);
    if (b->oom)
    {
        return;
    }
    memcpy(b->data + b->len, s, n);
    b->len += n;
    b->data[b->len] = '\0';
}

static void
_xml_buf_append(_xml_buf_t *b, const char *s)
{
    if (NULL == s)
    {
        return;
    }
    _xml_buf_append_len(b, s, strlen(s));
}

/* Escape for XML attribute values: <, >, &, ", '. */
static void
_xml_buf_append_attr(_xml_buf_t *b, const char *s)
{
    const char *p;

    if (NULL == s)
    {
        return;
    }
    for (p = s; *p; p++)
    {
        unsigned char c = (unsigned char)*p;
        switch (c)
        {
        case '<':
            _xml_buf_append(b, "&lt;");
            break;
        case '>':
            _xml_buf_append(b, "&gt;");
            break;
        case '&':
            _xml_buf_append(b, "&amp;");
            break;
        case '"':
            _xml_buf_append(b, "&quot;");
            break;
        case '\'':
            _xml_buf_append(b, "&apos;");
            break;
        case '\r':
        case '\n':
        case '\t':
            /* XML 1.0 forbids most control chars in attributes;
             * whitespace gets folded to a space so the rendered
             * attribute stays on one line. */
            _xml_buf_append(b, " ");
            break;
        default:
            if (c < 0x20)
            {
                /* Drop other control bytes; they aren't legal in
                 * XML 1.0 attributes and parsers reject them. */
                break;
            }
            _xml_buf_append_len(b, (const char *)&c, 1);
            break;
        }
    }
}

/* Escape for XML element text content: <, >, &. */
static void
_xml_buf_append_text(_xml_buf_t *b, const char *s)
{
    const char *p;

    if (NULL == s)
    {
        return;
    }
    for (p = s; *p; p++)
    {
        unsigned char c = (unsigned char)*p;
        switch (c)
        {
        case '<':
            _xml_buf_append(b, "&lt;");
            break;
        case '>':
            _xml_buf_append(b, "&gt;");
            break;
        case '&':
            _xml_buf_append(b, "&amp;");
            break;
        case '\r':
            /* Strip; the writer emits \n line endings only. */
            break;
        default:
            if (c < 0x20 && '\n' != c && '\t' != c)
            {
                break;
            }
            _xml_buf_append_len(b, (const char *)&c, 1);
            break;
        }
    }
}

static void
_xml_buf_append_double(_xml_buf_t *b, double v)
{
    char tmp[32];
    int n;

    n = snprintf(tmp, sizeof(tmp), "%.6f", v);
    if (n < 0)
    {
        return;
    }
    if ((size_t)n >= sizeof(tmp))
    {
        n = (int)sizeof(tmp) - 1;
    }
    _xml_buf_append_len(b, tmp, (size_t)n);
}

static void
_xml_buf_append_size(_xml_buf_t *b, size_t v)
{
    char tmp[32];
    int n;

    n = snprintf(tmp, sizeof(tmp), "%zu", v);
    if (n < 0)
    {
        return;
    }
    if ((size_t)n >= sizeof(tmp))
    {
        n = (int)sizeof(tmp) - 1;
    }
    _xml_buf_append_len(b, tmp, (size_t)n);
}

static void
_render_case(_xml_buf_t *b, const _junit_case_t *c, const char *fallback_classname)
{
    const char *classname;

    classname = (c->classname[0]) ? c->classname : fallback_classname;

    _xml_buf_append(b, "    <testcase classname=\"");
    _xml_buf_append_attr(b, classname);
    _xml_buf_append(b, "\" name=\"");
    _xml_buf_append_attr(b, c->name);
    _xml_buf_append(b, "\" time=\"");
    _xml_buf_append_double(b, c->time_sec);
    _xml_buf_append(b, "\"");

    switch (c->outcome)
    {
    case LFG_CT_PASSED:
        _xml_buf_append(b, "/>\n");
        break;

    case LFG_CT_FAILED:
        _xml_buf_append(b, ">\n");
        _xml_buf_append(b, "      <failure message=\"");
        _xml_buf_append_attr(b, c->has_message ? c->message : "assertion failed");
        _xml_buf_append(b, "\">");
        if (c->has_message)
        {
            _xml_buf_append_text(b, c->message);
        }
        _xml_buf_append(b, "</failure>\n");
        _xml_buf_append(b, "    </testcase>\n");
        break;

    case LFG_CT_SKIPPED:
        _xml_buf_append(b, ">\n");
        _xml_buf_append(b, "      <skipped");
        if (c->has_message)
        {
            _xml_buf_append(b, " message=\"");
            _xml_buf_append_attr(b, c->message);
            _xml_buf_append(b, "\"");
        }
        _xml_buf_append(b, "/>\n");
        _xml_buf_append(b, "    </testcase>\n");
        break;

    case LFG_CT_XFAIL:
        _xml_buf_append(b, ">\n");
        _xml_buf_append(b, "      <skipped type=\"xfail\" message=\"");
        _xml_buf_append_attr(b, c->has_message ? c->message : "expected failure");
        _xml_buf_append(b, "\"/>\n");
        _xml_buf_append(b, "    </testcase>\n");
        break;

    case LFG_CT_XPASS:
        _xml_buf_append(b, ">\n");
        _xml_buf_append(b, "      <skipped type=\"xpass\" message=\"");
        _xml_buf_append_attr(b, c->has_message ? c->message : "unexpected pass");
        _xml_buf_append(b, "\"/>\n");
        _xml_buf_append(b, "    </testcase>\n");
        break;

    default:
        _xml_buf_append(b, "/>\n");
        break;
    }
}
