/**
 * @file
 * @brief       Opt-in JUnit-XML report emitter for lfg-ctest.
 *
 * Installs itself as the runner's reporter (via @c lfg_ct_set_reporter)
 * and produces a Jenkins-flavored JUnit-XML document at the path
 * specified by the @c --output-junit @c \<path\> command-line flag (or
 * by an explicit @ref lfg_ct_junit_set_output call). The schema
 * matches the shape Gitea Actions, GitHub Actions @c dorny/test-reporter,
 * pytest @c --junit-xml, and @c gotestsum @c --junitfile all consume.
 *
 * This module is not part of the core lfg-ctest static library. Build
 * it via @c contrib/junit-xml/CMakeLists.txt (add_subdirectory) or
 * drop the @c .c / @c .h pair directly into your test binary.
 *
 * Typical wiring:
 *
 * @code
 *   #include <lfg-ctest.h>
 *   #include <lfg-ctest-junit.h>
 *
 *   int main(int argc, char *argv[])
 *   {
 *       argc = lfg_ct_junit_consume_args(argc, argv);
 *       if (lfg_ct_parse_args(argc, argv)) { return 1; }
 *       lfg_ct_start();
 *       ...
 *       lfg_ct_print_summary();  // also fires the junit flush
 *       return lfg_ct_return();
 *   }
 * @endcode
 */

#ifndef LFG_CTEST_JUNIT_H_
#define LFG_CTEST_JUNIT_H_

/** Scan @p argv for @c "--output-junit @c \<path\>". When found, peel
 *  the two consumed entries out of @p argv (shifting the remainder
 *  left by two), wire the junit emitter up as the active lfg-ctest
 *  reporter, and return the adjusted @p argc.
 *
 *  Call this @b before @ref lfg_ct_parse_args -- the core parser
 *  treats unknown flags as fatal, so @c --output-junit must be peeled
 *  off first.
 *
 *  The basename of @c argv[0] is captured as the testsuite's
 *  @c name attribute.
 *
 *  @return The new @p argc on success (possibly equal to the original
 *          if the flag was absent), or @c -1 if @c --output-junit
 *          appeared without a path argument (a usage message is
 *          written to stderr).
 */
int lfg_ct_junit_consume_args(int argc, char *argv[]);

/** Manual wiring. Bypasses the argv walk: install the junit reporter
 *  with explicit @p path and @p suite_name. Either @p path or
 *  @p suite_name may be @c NULL (a @c NULL @p path disables; a
 *  @c NULL @p suite_name falls back to a sensible default).
 *
 *  @c path is borrowed -- caller keeps the string alive until the
 *  reporter flushes (typically end of @c main).
 */
void lfg_ct_junit_set_output(const char *path, const char *suite_name);

#endif /* LFG_CTEST_JUNIT_H_ */
