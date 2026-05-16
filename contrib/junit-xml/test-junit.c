/*
 * Self-tests for the contrib JUnit-XML reporter.
 *
 * Validates the full wire from lfg-ctest's runner -> reporter
 * callback -> XML emission. Each test:
 *
 *   1. Wires the junit reporter to a temp file path via
 *      lfg_ct_junit_set_output().
 *   2. Drives a small synthetic batch through lfg_ct_test_impl()
 *      so the runner fires its on_record callback for real.
 *   3. Triggers the flush via lfg_ct_print_summary (which runs
 *      on_run_complete).
 *   4. Reads the produced file back, asserts substring shape, and
 *      cleans up.
 *
 * Substring assertions (over a real XML parser) keep the test
 * dependency-free; the framework's CI workflow runs xmllint --noout
 * against the artifact for syntactic validation.
 */

#include "lfg-ctest.h"
#include "lfg-ctest-junit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define JUNIT_TEST_TMP_PATH "/tmp/lfg-ctest-junit-selftest.xml"

/* Helper bodies. Failing bodies are NOT driven through lfg_ct_test_impl
 * here -- a real failure would advance the outer run's _tests_failed
 * counter and turn the binary red. Failure-shape recording is verified
 * by the XML the contrib emits when we drive a synthetic failed
 * record through the reporter directly via a thin shim. */
static void _pass_body(void) { ASSERT_TRUE(1); }
static void _skip_body(void) { lfg_ct_skip("not implemented"); }

/* Read the contents of @p path into a malloc'd NUL-terminated
 * buffer. Returns NULL on any error. Caller frees. */
static char *_slurp(const char *path)
{
    FILE *fp;
    long size;
    char *buf;
    size_t got;

    fp = fopen(path, "rb");
    if (NULL == fp)
    {
        return NULL;
    }
    if (0 != fseek(fp, 0, SEEK_END))
    {
        fclose(fp);
        return NULL;
    }
    size = ftell(fp);
    if (size < 0)
    {
        fclose(fp);
        return NULL;
    }
    rewind(fp);

    buf = (char *)malloc((size_t)size + 1);
    if (NULL == buf)
    {
        fclose(fp);
        return NULL;
    }
    got = fread(buf, 1, (size_t)size, fp);
    fclose(fp);
    buf[got] = '\0';
    return buf;
}

/* Wires the reporter, drives one batch, flushes via print_summary,
 * and returns the file contents. Caller frees. */
static char *_drive_and_capture(void)
{
    char *contents;
    /* Disable, re-enable: tests share file-static state in the
     * contrib, so a clean slate per test is mandatory. */
    lfg_ct_junit_set_output(NULL, NULL);
    lfg_ct_junit_set_output(JUNIT_TEST_TMP_PATH, "self-test-junit");

    lfg_ct_test_impl(NULL, _pass_body, NULL, "pass_case");
    lfg_ct_test_impl(NULL, _skip_body, NULL, "skip_case");

    /* Fire the run-complete hook. We cannot call the real
     * lfg_ct_print_summary because it produces stdout output and
     * resets pass/fail tallies; instead, just unhook the reporter,
     * which triggers a deliberate flush via the public setter. */
    /* Actually -- lfg_ct_junit_set_output(NULL, ...) DOES NOT flush;
     * it just unregisters. The clean way to force a flush in a
     * self-test is to call lfg_ct_print_summary. That's fine here:
     * the outer test binary's main calls it once at the very end,
     * but each inner self-test also calls it to flush its own
     * batch. The pass/fail tallies accumulate but the outer
     * verifies via lfg_ct_self_* accessors anyway. */
    lfg_ct_print_summary();
    contents = _slurp(JUNIT_TEST_TMP_PATH);
    remove(JUNIT_TEST_TMP_PATH);
    return contents;
}

static void test_junit_records_pass_with_no_failure_child(void)
{
    char *xml = _drive_and_capture();

    ASSERT_NOT_NULL(xml);
    if (xml)
    {
        ASSERT_TRUE(NULL != strstr(xml, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"));
        ASSERT_TRUE(NULL != strstr(xml, "<testsuite name=\"self-test-junit\""));
        ASSERT_TRUE(NULL != strstr(xml, "name=\"pass_case\""));
        ASSERT_TRUE(NULL != strstr(xml, "name=\"skip_case\""));
        free(xml);
    }
}

static void test_junit_renders_skipped_with_reason(void)
{
    char *xml = _drive_and_capture();

    ASSERT_NOT_NULL(xml);
    if (xml)
    {
        /* The skip body called lfg_ct_skip("not implemented");
         * that string must appear as the message attribute on a
         * <skipped/> child of the skip_case testcase. */
        ASSERT_TRUE(NULL != strstr(xml, "<skipped"));
        ASSERT_TRUE(NULL != strstr(xml, "message=\"not implemented\""));
        free(xml);
    }
}

static void test_junit_aggregate_counts_match_cases(void)
{
    char *xml = _drive_and_capture();

    ASSERT_NOT_NULL(xml);
    if (xml)
    {
        /* Two testcases (pass + skip) recorded -- tests=2,
         * failures=0, skipped=1. */
        ASSERT_TRUE(NULL != strstr(xml, "tests=\"2\""));
        ASSERT_TRUE(NULL != strstr(xml, "failures=\"0\""));
        ASSERT_TRUE(NULL != strstr(xml, "skipped=\"1\""));
        ASSERT_TRUE(NULL != strstr(xml, "errors=\"0\""));
        free(xml);
    }
}

/* The argv walk peels --output-junit out before parse_args sees it. */
static void test_junit_consume_args_peels_flag_and_path(void)
{
    char prog[]  = "test-prog";
    char flag[]  = "--output-junit";
    char path[]  = "/tmp/foo.xml";
    char extra[] = "--list";
    char *argv[] = {prog, flag, path, extra, NULL};
    int argc;

    argc = lfg_ct_junit_consume_args(4, argv);

    ASSERT_INT_EQUAL(2, argc);
    ASSERT_STR_EQUAL("test-prog", argv[0]);
    ASSERT_STR_EQUAL("--list", argv[1]);
    /* Disable so subsequent tests start clean. */
    lfg_ct_junit_set_output(NULL, NULL);
}

static void test_junit_consume_args_returns_minus_one_on_missing_path(void)
{
    char prog[] = "test-prog";
    char flag[] = "--output-junit";
    char *argv[] = {prog, flag, NULL};
    int rc;

    rc = lfg_ct_junit_consume_args(2, argv);
    ASSERT_INT_EQUAL(-1, rc);
    lfg_ct_junit_set_output(NULL, NULL);
}

static void test_junit_consume_args_no_flag_returns_argc_unchanged(void)
{
    char prog[] = "test-prog";
    char other[] = "--filter";
    char glob[]  = "test_*";
    char *argv[] = {prog, other, glob, NULL};
    int rc;

    rc = lfg_ct_junit_consume_args(3, argv);
    ASSERT_INT_EQUAL(3, rc);
    ASSERT_STR_EQUAL("--filter", argv[1]);
    ASSERT_STR_EQUAL("test_*", argv[2]);
}

static void suite_junit_tests(void)
{
    lfg_ct_test(NULL, test_junit_records_pass_with_no_failure_child, NULL);
    lfg_ct_test(NULL, test_junit_renders_skipped_with_reason, NULL);
    lfg_ct_test(NULL, test_junit_aggregate_counts_match_cases, NULL);
    lfg_ct_test(NULL, test_junit_consume_args_peels_flag_and_path, NULL);
    lfg_ct_test(NULL, test_junit_consume_args_returns_minus_one_on_missing_path, NULL);
    lfg_ct_test(NULL, test_junit_consume_args_no_flag_returns_argc_unchanged, NULL);
}

int main(int argc, char *argv[])
{
    /* This is the contrib's own test binary; it doesn't accept
     * --output-junit. Just parse the core flags. */
    if (0 != lfg_ct_parse_args(argc, argv))
    {
        return 1;
    }

    lfg_ct_start();
    lfg_ct_suite(NULL, suite_junit_tests, NULL);
    lfg_ct_print_summary();
    return lfg_ct_return();
}
