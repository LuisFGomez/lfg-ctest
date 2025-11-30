/**
 * @file
 * @brief       Test suite for LFG mocking framework
 */

#include "lfgtest.h"
#include "lfgtest_mock.h"
#include <stdint.h>

/*============================================================================
 *  Mock Declarations (in header, typically)
 *==========================================================================*/

/* V_V: void return, no params */
DECLARE_MOCK_V_V(simple_void_func);

/* R_V: returns value, no params */
DECLARE_MOCK_R_V(get_value, int);

/* V_1: void return, 1 param */
DECLARE_MOCK_V_1(set_value, int);

/* R_1: returns value, 1 param */
DECLARE_MOCK_R_1(increment, int, int);

/* V_2: void return, 2 params */
DECLARE_MOCK_V_2(copy_data, void*, size_t);

/* R_2: returns value, 2 params */
DECLARE_MOCK_R_2(add_numbers, int, int, int);

/* R_3: returns value, 3 params - typical for functions with output params */
DECLARE_MOCK_R_3(read_buffer, int, uint8_t*, size_t, size_t*);

/* V_3: void return, 3 params */
DECLARE_MOCK_V_3(configure, int, int, int);

/* R_4: returns value, 4 params */
DECLARE_MOCK_R_4(transfer, int, void*, size_t, void*, size_t);

/*============================================================================
 *  Struct-by-value Mock Declarations
 *  Note: param actions (mock_param_mem_read/write) don't work with struct
 *  params, but param_history and return_queue work fine.
 *==========================================================================*/

struct point
{
    int x;
    int y;
};

struct rect
{
    struct point origin;
    struct point size;
};

/* V_1_S with struct param (struct-safe, no param actions) */
DECLARE_MOCK_V_1_S(draw_point, struct point);

/* V_2_S with struct param and primitive */
DECLARE_MOCK_V_2_S(draw_rect, struct rect, int);

/* R_V_S returning a struct */
DECLARE_MOCK_R_V_S(get_origin, struct point);

/* R_1_S with struct param and struct return */
DECLARE_MOCK_R_1_S(transform_point, struct point, struct point);

/* V_3_S with multiple struct params */
DECLARE_MOCK_V_3_S(draw_line, struct point, struct point, int);

/*============================================================================
 *  Mock Definitions (in .c file, typically)
 *==========================================================================*/

DEFINE_MOCK_V_V(simple_void_func)
DEFINE_MOCK_R_V(get_value, int)
DEFINE_MOCK_V_1(set_value, int)
DEFINE_MOCK_R_1(increment, int, int)
DEFINE_MOCK_V_2(copy_data, void*, size_t)
DEFINE_MOCK_R_2(add_numbers, int, int, int)
DEFINE_MOCK_R_3(read_buffer, int, uint8_t*, size_t, size_t*)
DEFINE_MOCK_V_3(configure, int, int, int)
DEFINE_MOCK_R_4(transfer, int, void*, size_t, void*, size_t)

/* Struct-by-value mock definitions (using _S variants) */
DEFINE_MOCK_V_1_S(draw_point, struct point)
DEFINE_MOCK_V_2_S(draw_rect, struct rect, int)
DEFINE_MOCK_R_V_S(get_origin, struct point)
DEFINE_MOCK_R_1_S(transform_point, struct point, struct point)
DEFINE_MOCK_V_3_S(draw_line, struct point, struct point, int)

/*============================================================================
 *  Test: V_V - void return, no params
 *==========================================================================*/

static int test_mock_v_v(void)
{
    simple_void_func__mock_reset();

    ASSERT_INT_EQUAL(0, simple_void_func__call_count);

    simple_void_func__mock();
    ASSERT_INT_EQUAL(1, simple_void_func__call_count);

    simple_void_func__mock();
    simple_void_func__mock();
    ASSERT_INT_EQUAL(3, simple_void_func__call_count);

    simple_void_func__mock_reset();
    ASSERT_INT_EQUAL(0, simple_void_func__call_count);
    return lft_current_test_return();
}

/*============================================================================
 *  Test: R_V - returns value, no params
 *==========================================================================*/

static int test_mock_r_v(void)
{
    int result;

    get_value__mock_reset();

    /* Set up return queue */
    get_value__return_queue[0] = 42;
    get_value__return_queue[1] = 100;
    get_value__return_queue[2] = -5;

    result = get_value__mock();
    ASSERT_INT_EQUAL(42, result);
    ASSERT_INT_EQUAL(1, get_value__call_count);

    result = get_value__mock();
    ASSERT_INT_EQUAL(100, result);

    result = get_value__mock();
    ASSERT_INT_EQUAL(-5, result);

    ASSERT_INT_EQUAL(3, get_value__call_count);

    get_value__mock_reset();
    ASSERT_INT_EQUAL(0, get_value__call_count);
    return lft_current_test_return();
}

/*============================================================================
 *  Test: V_1 - void return, 1 param
 *==========================================================================*/

static int test_mock_v_1(void)
{
    set_value__mock_reset();

    set_value__mock(42);
    set_value__mock(100);
    set_value__mock(-5);

    ASSERT_INT_EQUAL(3, set_value__call_count);
    ASSERT_INT_EQUAL(42, set_value__param_history[0].p0);
    ASSERT_INT_EQUAL(100, set_value__param_history[1].p0);
    ASSERT_INT_EQUAL(-5, set_value__param_history[2].p0);

    set_value__mock_reset();
    ASSERT_INT_EQUAL(0, set_value__call_count);
    return lft_current_test_return();
}

/*============================================================================
 *  Test: R_1 - returns value, 1 param
 *==========================================================================*/

static int test_mock_r_1(void)
{
    int result;

    increment__mock_reset();

    increment__return_queue[0] = 11;
    increment__return_queue[1] = 21;

    result = increment__mock(10);
    ASSERT_INT_EQUAL(11, result);
    ASSERT_INT_EQUAL(10, increment__param_history[0].p0);

    result = increment__mock(20);
    ASSERT_INT_EQUAL(21, result);
    ASSERT_INT_EQUAL(20, increment__param_history[1].p0);

    ASSERT_INT_EQUAL(2, increment__call_count);

    increment__mock_reset();
    return lft_current_test_return();
}

/*============================================================================
 *  Test: V_2 - void return, 2 params
 *==========================================================================*/

static int test_mock_v_2(void)
{
    char buf1[] = "hello";
    char buf2[] = "world";

    copy_data__mock_reset();

    copy_data__mock(buf1, sizeof(buf1));
    copy_data__mock(buf2, sizeof(buf2));

    ASSERT_INT_EQUAL(2, copy_data__call_count);
    ASSERT_PTR_EQUAL(buf1, copy_data__param_history[0].p0);
    ASSERT_INT_EQUAL(sizeof(buf1), copy_data__param_history[0].p1);
    ASSERT_PTR_EQUAL(buf2, copy_data__param_history[1].p0);
    ASSERT_INT_EQUAL(sizeof(buf2), copy_data__param_history[1].p1);

    copy_data__mock_reset();
    return lft_current_test_return();
}

/*============================================================================
 *  Test: R_2 - returns value, 2 params
 *==========================================================================*/

static int test_mock_r_2(void)
{
    int result;

    add_numbers__mock_reset();

    add_numbers__return_queue[0] = 30;
    add_numbers__return_queue[1] = 15;

    result = add_numbers__mock(10, 20);
    ASSERT_INT_EQUAL(30, result);
    ASSERT_INT_EQUAL(10, add_numbers__param_history[0].p0);
    ASSERT_INT_EQUAL(20, add_numbers__param_history[0].p1);

    result = add_numbers__mock(5, 10);
    ASSERT_INT_EQUAL(15, result);
    ASSERT_INT_EQUAL(5, add_numbers__param_history[1].p0);
    ASSERT_INT_EQUAL(10, add_numbers__param_history[1].p1);

    add_numbers__mock_reset();
    return lft_current_test_return();
}

/*============================================================================
 *  Test: V_3 - void return, 3 params
 *==========================================================================*/

static int test_mock_v_3(void)
{
    configure__mock_reset();

    configure__mock(1, 2, 3);
    configure__mock(10, 20, 30);

    ASSERT_INT_EQUAL(2, configure__call_count);

    ASSERT_INT_EQUAL(1, configure__param_history[0].p0);
    ASSERT_INT_EQUAL(2, configure__param_history[0].p1);
    ASSERT_INT_EQUAL(3, configure__param_history[0].p2);

    ASSERT_INT_EQUAL(10, configure__param_history[1].p0);
    ASSERT_INT_EQUAL(20, configure__param_history[1].p1);
    ASSERT_INT_EQUAL(30, configure__param_history[1].p2);

    configure__mock_reset();
    return lft_current_test_return();
}

/*============================================================================
 *  Test: R_4 - returns value, 4 params
 *==========================================================================*/

static int test_mock_r_4(void)
{
    char src[] = "source";
    char dst[16];
    int result;

    transfer__mock_reset();

    transfer__return_queue[0] = 0;  /* success */
    transfer__return_queue[1] = -1; /* error */

    result = transfer__mock(dst, sizeof(dst), src, sizeof(src));
    ASSERT_INT_EQUAL(0, result);
    ASSERT_PTR_EQUAL(dst, transfer__param_history[0].p0);
    ASSERT_INT_EQUAL(sizeof(dst), transfer__param_history[0].p1);
    ASSERT_PTR_EQUAL(src, transfer__param_history[0].p2);
    ASSERT_INT_EQUAL(sizeof(src), transfer__param_history[0].p3);

    result = transfer__mock(NULL, 0, NULL, 0);
    ASSERT_INT_EQUAL(-1, result);

    transfer__mock_reset();
    return lft_current_test_return();
}

/*============================================================================
 *  Test: param action - mem_read (capture data from param)
 *==========================================================================*/

static int test_mock_param_action_read(void)
{
    uint8_t captured_buf[8] = {0};
    uint8_t test_data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    mock_param_action_t action;

    copy_data__mock_reset();

    /* Set up action to capture bytes from param 0 on call 0 */
    action = mock_param_mem_read(NULL, 0, 0, captured_buf, 4);
    copy_data__param_actions = action;

    /* Call mock with test data */
    copy_data__mock(test_data, sizeof(test_data));

    /* Verify data was captured */
    ASSERT_MEM_EQUAL(test_data, captured_buf, 4);

    copy_data__mock_reset();
    return lft_current_test_return();
}

/*============================================================================
 *  Test: param action - mem_write (inject data into param)
 *==========================================================================*/

static int test_mock_param_action_write(void)
{
    uint8_t output_buf[8] = {0};
    uint8_t inject_data[] = {0xCA, 0xFE, 0xBA, 0xBE};
    mock_param_action_t action;

    copy_data__mock_reset();

    /* Set up action to inject bytes into param 0 on call 0 */
    action = mock_param_mem_write(NULL, 0, 0, inject_data, 4);
    copy_data__param_actions = action;

    /* Call mock - output_buf should receive injected data */
    copy_data__mock(output_buf, sizeof(output_buf));

    /* Verify data was injected */
    ASSERT_MEM_EQUAL(inject_data, output_buf, 4);

    copy_data__mock_reset();
    return lft_current_test_return();
}

/*============================================================================
 *  Test: param action - multiple calls with different actions
 *==========================================================================*/

static int test_mock_param_action_multi_call(void)
{
    uint8_t captured1[4] = {0};
    uint8_t captured2[4] = {0};
    uint8_t data1[] = {0x11, 0x22, 0x33, 0x44};
    uint8_t data2[] = {0xAA, 0xBB, 0xCC, 0xDD};
    mock_param_action_t action;

    copy_data__mock_reset();

    /* Set up action chain: capture from call 0, then from call 1 */
    action = mock_param_mem_read(NULL, 0, 0, captured1, 4);
    action = mock_param_mem_read(action, 1, 0, captured2, 4);
    copy_data__param_actions = action;

    /* Make two calls */
    copy_data__mock(data1, sizeof(data1));
    copy_data__mock(data2, sizeof(data2));

    /* Verify correct data captured from each call */
    ASSERT_MEM_EQUAL(data1, captured1, 4);
    ASSERT_MEM_EQUAL(data2, captured2, 4);

    copy_data__mock_reset();
    return lft_current_test_return();
}

/*============================================================================
 *  Test: R_3 with output parameter simulation
 *==========================================================================*/

static int test_mock_r_3_output_param(void)
{
    uint8_t buffer[16];
    size_t bytes_read = 0;
    size_t inject_bytes_read = 10;
    int result;
    mock_param_action_t action;

    read_buffer__mock_reset();

    /* Set up return value */
    read_buffer__return_queue[0] = 0; /* success */

    /* Inject value into the output parameter (p2 = size_t*) */
    action = mock_param_mem_write(NULL, 0, 2, &inject_bytes_read, sizeof(size_t));
    read_buffer__param_actions = action;

    /* Call the mock */
    result = read_buffer__mock(buffer, sizeof(buffer), &bytes_read);

    /* Verify */
    ASSERT_INT_EQUAL(0, result);
    ASSERT_INT_EQUAL(10, bytes_read);
    ASSERT_PTR_EQUAL(buffer, read_buffer__param_history[0].p0);
    ASSERT_INT_EQUAL(sizeof(buffer), read_buffer__param_history[0].p1);

    read_buffer__mock_reset();
    return lft_current_test_return();
}

/*============================================================================
 *  Test: Pointer address vs dereferenced memory capture
 *  Demonstrates the difference between:
 *  - param_history: captures the pointer VALUE (address)
 *  - mock_param_mem_read: captures the CONTENTS at that address
 *==========================================================================*/

static int test_mock_pointer_vs_memory(void)
{
    uint8_t data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    uint8_t captured_contents[4] = {0};
    void *captured_address;
    mock_param_action_t action;

    copy_data__mock_reset();

    /* Set up to capture memory contents from param 0 */
    action = mock_param_mem_read(NULL, 0, 0, captured_contents, 4);
    copy_data__param_actions = action;

    /* Call mock */
    copy_data__mock(data, sizeof(data));

    /* param_history captures the POINTER (address) */
    captured_address = copy_data__param_history[0].p0;
    ASSERT_PTR_EQUAL(data, captured_address);

    /* mock_param_mem_read captures the CONTENTS (dereferenced) */
    ASSERT_MEM_EQUAL(data, captured_contents, 4);

    /* They are fundamentally different:
     * - captured_address == &data[0] (where the data lives)
     * - captured_contents == {0xDE, 0xAD, 0xBE, 0xEF} (copy of the data)
     */
    ASSERT_PTR_NOT_EQUAL(captured_address, captured_contents);

    copy_data__mock_reset();
    return lft_current_test_return();
}

/*============================================================================
 *  Test: Verify reset clears everything
 *==========================================================================*/

static int test_mock_reset_clears_all(void)
{
    mock_param_action_t action;

    add_numbers__mock_reset();

    /* Set up state */
    add_numbers__return_queue[0] = 999;
    add_numbers__mock(1, 2);
    action = mock_param_mem_read(NULL, 0, 0, NULL, 0);
    add_numbers__param_actions = action;

    ASSERT_INT_EQUAL(1, add_numbers__call_count);
    ASSERT_INT_EQUAL(1, add_numbers__param_history[0].p0);

    /* Reset */
    add_numbers__mock_reset();

    /* Verify everything cleared */
    ASSERT_INT_EQUAL(0, add_numbers__call_count);
    ASSERT_INT_EQUAL(0, add_numbers__return_queue[0]);
    ASSERT_INT_EQUAL(0, add_numbers__param_history[0].p0);
    ASSERT_NULL(add_numbers__param_actions);
    return lft_current_test_return();
}

/*============================================================================
 *  Test: Struct-by-value parameter capture
 *==========================================================================*/

static int test_mock_struct_param(void)
{
    struct point p = {10, 20};

    draw_point__mock_reset();

    draw_point__mock(p);

    ASSERT_INT_EQUAL(1, draw_point__call_count);
    ASSERT_INT_EQUAL(10, draw_point__param_history[0].p0.x);
    ASSERT_INT_EQUAL(20, draw_point__param_history[0].p0.y);

    /* Modify original - captured copy should be unaffected */
    p.x = 999;
    ASSERT_INT_EQUAL(10, draw_point__param_history[0].p0.x);

    draw_point__mock_reset();
    return lft_current_test_return();
}

/*============================================================================
 *  Test: Nested struct-by-value parameter
 *==========================================================================*/

static int test_mock_nested_struct_param(void)
{
    struct rect r;

    draw_rect__mock_reset();

    r.origin.x = 10;
    r.origin.y = 20;
    r.size.x = 100;
    r.size.y = 200;

    draw_rect__mock(r, 0xFF);

    ASSERT_INT_EQUAL(1, draw_rect__call_count);
    ASSERT_INT_EQUAL(10, draw_rect__param_history[0].p0.origin.x);
    ASSERT_INT_EQUAL(20, draw_rect__param_history[0].p0.origin.y);
    ASSERT_INT_EQUAL(100, draw_rect__param_history[0].p0.size.x);
    ASSERT_INT_EQUAL(200, draw_rect__param_history[0].p0.size.y);
    ASSERT_INT_EQUAL(0xFF, draw_rect__param_history[0].p1);

    draw_rect__mock_reset();
    return lft_current_test_return();
}

/*============================================================================
 *  Test: Struct return value (no params)
 *==========================================================================*/

static int test_mock_struct_return(void)
{
    struct point result;

    get_origin__mock_reset();

    /* Set up return value */
    get_origin__return_queue[0].x = 42;
    get_origin__return_queue[0].y = 84;

    result = get_origin__mock();

    ASSERT_INT_EQUAL(1, get_origin__call_count);
    ASSERT_INT_EQUAL(42, result.x);
    ASSERT_INT_EQUAL(84, result.y);

    get_origin__mock_reset();
    return lft_current_test_return();
}

/*============================================================================
 *  Test: Struct param and struct return
 *==========================================================================*/

static int test_mock_struct_param_and_return(void)
{
    struct point input = {10, 20};
    struct point output;

    transform_point__mock_reset();

    /* Set up return value */
    transform_point__return_queue[0].x = 100;
    transform_point__return_queue[0].y = 200;

    output = transform_point__mock(input);

    /* Verify param captured */
    ASSERT_INT_EQUAL(1, transform_point__call_count);
    ASSERT_INT_EQUAL(10, transform_point__param_history[0].p0.x);
    ASSERT_INT_EQUAL(20, transform_point__param_history[0].p0.y);

    /* Verify return value */
    ASSERT_INT_EQUAL(100, output.x);
    ASSERT_INT_EQUAL(200, output.y);

    transform_point__mock_reset();
    return lft_current_test_return();
}

/*============================================================================
 *  Test: Multiple struct params
 *==========================================================================*/

static int test_mock_multiple_struct_params(void)
{
    struct point p1 = {0, 0};
    struct point p2 = {100, 100};

    draw_line__mock_reset();

    draw_line__mock(p1, p2, 3);

    ASSERT_INT_EQUAL(1, draw_line__call_count);
    ASSERT_INT_EQUAL(0, draw_line__param_history[0].p0.x);
    ASSERT_INT_EQUAL(0, draw_line__param_history[0].p0.y);
    ASSERT_INT_EQUAL(100, draw_line__param_history[0].p1.x);
    ASSERT_INT_EQUAL(100, draw_line__param_history[0].p1.y);
    ASSERT_INT_EQUAL(3, draw_line__param_history[0].p2);

    draw_line__mock_reset();
    return lft_current_test_return();
}

/*============================================================================
 *  Test: Multiple calls with struct returns
 *==========================================================================*/

static int test_mock_struct_return_queue(void)
{
    struct point r1, r2, r3;

    get_origin__mock_reset();

    /* Queue up multiple return values */
    get_origin__return_queue[0].x = 1;
    get_origin__return_queue[0].y = 2;
    get_origin__return_queue[1].x = 10;
    get_origin__return_queue[1].y = 20;
    get_origin__return_queue[2].x = 100;
    get_origin__return_queue[2].y = 200;

    r1 = get_origin__mock();
    r2 = get_origin__mock();
    r3 = get_origin__mock();

    ASSERT_INT_EQUAL(3, get_origin__call_count);
    ASSERT_INT_EQUAL(1, r1.x);
    ASSERT_INT_EQUAL(2, r1.y);
    ASSERT_INT_EQUAL(10, r2.x);
    ASSERT_INT_EQUAL(20, r2.y);
    ASSERT_INT_EQUAL(100, r3.x);
    ASSERT_INT_EQUAL(200, r3.y);

    get_origin__mock_reset();
    return lft_current_test_return();
}

/*============================================================================
 *  Test Suite
 *==========================================================================*/

static int suite_mock_basic(void)
{
    lfgtest(test_mock_v_v);
    lfgtest(test_mock_r_v);
    lfgtest(test_mock_v_1);
    lfgtest(test_mock_r_1);
    lfgtest(test_mock_v_2);
    lfgtest(test_mock_r_2);
    lfgtest(test_mock_v_3);
    lfgtest(test_mock_r_4);
    return 0;
}

static int suite_mock_struct_by_value(void)
{
    lfgtest(test_mock_struct_param);
    lfgtest(test_mock_nested_struct_param);
    lfgtest(test_mock_struct_return);
    lfgtest(test_mock_struct_param_and_return);
    lfgtest(test_mock_multiple_struct_params);
    lfgtest(test_mock_struct_return_queue);
    return 0;
}

static int suite_mock_param_actions(void)
{
    lfgtest(test_mock_param_action_read);
    lfgtest(test_mock_param_action_write);
    lfgtest(test_mock_param_action_multi_call);
    lfgtest(test_mock_r_3_output_param);
    lfgtest(test_mock_pointer_vs_memory);
    lfgtest(test_mock_reset_clears_all);
    return 0;
}

/*============================================================================
 *  Main
 *==========================================================================*/

int main(void)
{
    lft_start();

    printf("\n");
    printf("================================================================================\n");
    printf("                    LFG MOCK FRAMEWORK TEST SUITE\n");
    printf("================================================================================\n\n");

    printf("--- SUITE 1: Basic Mock Operations ---\n");
    lft_suite(suite_mock_basic);

    printf("\n--- SUITE 2: Struct-by-Value ---\n");
    lft_suite(suite_mock_struct_by_value);

    printf("\n--- SUITE 3: Parameter Actions ---\n");
    lft_suite(suite_mock_param_actions);

    printf("\n");
    lft_print_summary();

    return lft_return();
}
