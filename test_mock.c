/**
 * @file
 * @brief       Test suite for lfg-ctest mocking framework
 */

#include "lfg_ctest.h"
#include "lfg_ctest_mock.h"
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

/* R_5: returns value, 5 params */
DECLARE_MOCK_R_5(do_r5, int, int, int, int, int, int);

/* R_6: returns value, 6 params */
DECLARE_MOCK_R_6(do_r6, int, int, int, int, int, int, int);

/* R_7: returns value, 7 params */
DECLARE_MOCK_R_7(do_r7, int, int, int, int, int, int, int, int);

/* R_8: returns value, 8 params */
DECLARE_MOCK_R_8(do_r8, int, int, int, int, int, int, int, int, int);

/* R_9: returns value, 9 params */
DECLARE_MOCK_R_9(do_r9, int, int, int, int, int, int, int, int, int, int);

/* V_4: void return, 4 params */
DECLARE_MOCK_V_4(do_v4, int, int, int, int);

/* V_5: void return, 5 params */
DECLARE_MOCK_V_5(do_v5, int, int, int, int, int);

/* V_6: void return, 6 params */
DECLARE_MOCK_V_6(do_v6, int, int, int, int, int, int);

/* V_7: void return, 7 params */
DECLARE_MOCK_V_7(do_v7, int, int, int, int, int, int, int);

/* V_8: void return, 8 params */
DECLARE_MOCK_V_8(do_v8, int, int, int, int, int, int, int, int);

/* V_9: void return, 9 params */
DECLARE_MOCK_V_9(do_v9, int, int, int, int, int, int, int, int, int);

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

/* R_2_S: returns value, 2 struct params */
DECLARE_MOCK_R_2_S(blend_points, struct point, struct point, struct point);

/* R_3_S: returns value, 3 params (mix of struct and primitive) */
DECLARE_MOCK_R_3_S(scale_point, struct point, struct point, int, int);

/* R_4_S: returns value, 4 params */
DECLARE_MOCK_R_4_S(compose_rect, struct rect, struct point, struct point, int, int);

/* R_5_S: returns value, 5 params */
DECLARE_MOCK_R_5_S(do_r5s, int, struct point, struct point, int, int, int);

/* R_6_S: returns value, 6 params */
DECLARE_MOCK_R_6_S(do_r6s, int, struct point, struct point, int, int, int, int);

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
DEFINE_MOCK_R_5(do_r5, int, int, int, int, int, int)
DEFINE_MOCK_R_6(do_r6, int, int, int, int, int, int, int)
DEFINE_MOCK_R_7(do_r7, int, int, int, int, int, int, int, int)
DEFINE_MOCK_R_8(do_r8, int, int, int, int, int, int, int, int, int)
DEFINE_MOCK_R_9(do_r9, int, int, int, int, int, int, int, int, int, int)
DEFINE_MOCK_V_4(do_v4, int, int, int, int)
DEFINE_MOCK_V_5(do_v5, int, int, int, int, int)
DEFINE_MOCK_V_6(do_v6, int, int, int, int, int, int)
DEFINE_MOCK_V_7(do_v7, int, int, int, int, int, int, int)
DEFINE_MOCK_V_8(do_v8, int, int, int, int, int, int, int, int)
DEFINE_MOCK_V_9(do_v9, int, int, int, int, int, int, int, int, int)

/* Struct-by-value mock definitions (using _S variants) */
DEFINE_MOCK_V_1_S(draw_point, struct point)
DEFINE_MOCK_V_2_S(draw_rect, struct rect, int)
DEFINE_MOCK_R_V_S(get_origin, struct point)
DEFINE_MOCK_R_1_S(transform_point, struct point, struct point)
DEFINE_MOCK_V_3_S(draw_line, struct point, struct point, int)
DEFINE_MOCK_R_2_S(blend_points, struct point, struct point, struct point)
DEFINE_MOCK_R_3_S(scale_point, struct point, struct point, int, int)
DEFINE_MOCK_R_4_S(compose_rect, struct rect, struct point, struct point, int, int)
DEFINE_MOCK_R_5_S(do_r5s, int, struct point, struct point, int, int, int)
DEFINE_MOCK_R_6_S(do_r6s, int, struct point, struct point, int, int, int, int)

/*============================================================================
 *  Test: V_V - void return, no params
 *==========================================================================*/

static void test_mock_v_v(void)
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
}

/*============================================================================
 *  Test: R_V - returns value, no params
 *==========================================================================*/

static void test_mock_r_v(void)
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
}

/*============================================================================
 *  Test: V_1 - void return, 1 param
 *==========================================================================*/

static void test_mock_v_1(void)
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
}

/*============================================================================
 *  Test: R_1 - returns value, 1 param
 *==========================================================================*/

static void test_mock_r_1(void)
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
}

/*============================================================================
 *  Test: V_2 - void return, 2 params
 *==========================================================================*/

static void test_mock_v_2(void)
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
}

/*============================================================================
 *  Test: R_2 - returns value, 2 params
 *==========================================================================*/

static void test_mock_r_2(void)
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
}

/*============================================================================
 *  Test: V_3 - void return, 3 params
 *==========================================================================*/

static void test_mock_v_3(void)
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
}

/*============================================================================
 *  Test: R_4 - returns value, 4 params
 *==========================================================================*/

static void test_mock_r_4(void)
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
}

/*============================================================================
 *  Test: R_3 - returns value, 3 params (basic usage)
 *==========================================================================*/

static void test_mock_r_3(void)
{
    uint8_t buf[16];
    size_t out = 0;
    int result;

    read_buffer__mock_reset();

    read_buffer__return_queue[0] = 0;
    read_buffer__return_queue[1] = -1;

    result = read_buffer__mock(buf, sizeof(buf), &out);
    ASSERT_INT_EQUAL(0, result);
    ASSERT_PTR_EQUAL(buf, read_buffer__param_history[0].p0);
    ASSERT_INT_EQUAL(sizeof(buf), read_buffer__param_history[0].p1);
    ASSERT_PTR_EQUAL(&out, read_buffer__param_history[0].p2);

    result = read_buffer__mock(NULL, 0, NULL);
    ASSERT_INT_EQUAL(-1, result);
    ASSERT_INT_EQUAL(2, read_buffer__call_count);

    read_buffer__mock_reset();
}

/*============================================================================
 *  Test: R_5 - returns value, 5 params
 *==========================================================================*/

static void test_mock_r_5(void)
{
    int result;

    do_r5__mock_reset();

    do_r5__return_queue[0] = 55;

    result = do_r5__mock(1, 2, 3, 4, 5);
    ASSERT_INT_EQUAL(55, result);
    ASSERT_INT_EQUAL(1, do_r5__param_history[0].p0);
    ASSERT_INT_EQUAL(2, do_r5__param_history[0].p1);
    ASSERT_INT_EQUAL(3, do_r5__param_history[0].p2);
    ASSERT_INT_EQUAL(4, do_r5__param_history[0].p3);
    ASSERT_INT_EQUAL(5, do_r5__param_history[0].p4);
    ASSERT_INT_EQUAL(1, do_r5__call_count);

    do_r5__mock_reset();
}

/*============================================================================
 *  Test: R_6 - returns value, 6 params
 *==========================================================================*/

static void test_mock_r_6(void)
{
    int result;

    do_r6__mock_reset();

    do_r6__return_queue[0] = 66;

    result = do_r6__mock(10, 20, 30, 40, 50, 60);
    ASSERT_INT_EQUAL(66, result);
    ASSERT_INT_EQUAL(10, do_r6__param_history[0].p0);
    ASSERT_INT_EQUAL(20, do_r6__param_history[0].p1);
    ASSERT_INT_EQUAL(30, do_r6__param_history[0].p2);
    ASSERT_INT_EQUAL(40, do_r6__param_history[0].p3);
    ASSERT_INT_EQUAL(50, do_r6__param_history[0].p4);
    ASSERT_INT_EQUAL(60, do_r6__param_history[0].p5);

    do_r6__mock_reset();
}

/*============================================================================
 *  Test: R_7 - returns value, 7 params
 *==========================================================================*/

static void test_mock_r_7(void)
{
    int result;

    do_r7__mock_reset();

    do_r7__return_queue[0] = 77;

    result = do_r7__mock(1, 2, 3, 4, 5, 6, 7);
    ASSERT_INT_EQUAL(77, result);
    ASSERT_INT_EQUAL(1, do_r7__param_history[0].p0);
    ASSERT_INT_EQUAL(4, do_r7__param_history[0].p3);
    ASSERT_INT_EQUAL(7, do_r7__param_history[0].p6);

    do_r7__mock_reset();
}

/*============================================================================
 *  Test: R_8 - returns value, 8 params
 *==========================================================================*/

static void test_mock_r_8(void)
{
    int result;

    do_r8__mock_reset();

    do_r8__return_queue[0] = 88;

    result = do_r8__mock(1, 2, 3, 4, 5, 6, 7, 8);
    ASSERT_INT_EQUAL(88, result);
    ASSERT_INT_EQUAL(1, do_r8__param_history[0].p0);
    ASSERT_INT_EQUAL(5, do_r8__param_history[0].p4);
    ASSERT_INT_EQUAL(8, do_r8__param_history[0].p7);

    do_r8__mock_reset();
}

/*============================================================================
 *  Test: R_9 - returns value, 9 params
 *==========================================================================*/

static void test_mock_r_9(void)
{
    int result;

    do_r9__mock_reset();

    do_r9__return_queue[0] = 99;

    result = do_r9__mock(1, 2, 3, 4, 5, 6, 7, 8, 9);
    ASSERT_INT_EQUAL(99, result);
    ASSERT_INT_EQUAL(1, do_r9__param_history[0].p0);
    ASSERT_INT_EQUAL(5, do_r9__param_history[0].p4);
    ASSERT_INT_EQUAL(9, do_r9__param_history[0].p8);

    do_r9__mock_reset();
}

/*============================================================================
 *  Test: V_4 - void return, 4 params
 *==========================================================================*/

static void test_mock_v_4(void)
{
    do_v4__mock_reset();

    do_v4__mock(10, 20, 30, 40);

    ASSERT_INT_EQUAL(1, do_v4__call_count);
    ASSERT_INT_EQUAL(10, do_v4__param_history[0].p0);
    ASSERT_INT_EQUAL(20, do_v4__param_history[0].p1);
    ASSERT_INT_EQUAL(30, do_v4__param_history[0].p2);
    ASSERT_INT_EQUAL(40, do_v4__param_history[0].p3);

    do_v4__mock_reset();
}

/*============================================================================
 *  Test: V_5 - void return, 5 params
 *==========================================================================*/

static void test_mock_v_5(void)
{
    do_v5__mock_reset();

    do_v5__mock(1, 2, 3, 4, 5);

    ASSERT_INT_EQUAL(1, do_v5__call_count);
    ASSERT_INT_EQUAL(1, do_v5__param_history[0].p0);
    ASSERT_INT_EQUAL(5, do_v5__param_history[0].p4);

    do_v5__mock_reset();
}

/*============================================================================
 *  Test: V_6 - void return, 6 params
 *==========================================================================*/

static void test_mock_v_6(void)
{
    do_v6__mock_reset();

    do_v6__mock(1, 2, 3, 4, 5, 6);

    ASSERT_INT_EQUAL(1, do_v6__call_count);
    ASSERT_INT_EQUAL(1, do_v6__param_history[0].p0);
    ASSERT_INT_EQUAL(6, do_v6__param_history[0].p5);

    do_v6__mock_reset();
}

/*============================================================================
 *  Test: V_7 - void return, 7 params
 *==========================================================================*/

static void test_mock_v_7(void)
{
    do_v7__mock_reset();

    do_v7__mock(1, 2, 3, 4, 5, 6, 7);

    ASSERT_INT_EQUAL(1, do_v7__call_count);
    ASSERT_INT_EQUAL(1, do_v7__param_history[0].p0);
    ASSERT_INT_EQUAL(7, do_v7__param_history[0].p6);

    do_v7__mock_reset();
}

/*============================================================================
 *  Test: V_8 - void return, 8 params
 *==========================================================================*/

static void test_mock_v_8(void)
{
    do_v8__mock_reset();

    do_v8__mock(1, 2, 3, 4, 5, 6, 7, 8);

    ASSERT_INT_EQUAL(1, do_v8__call_count);
    ASSERT_INT_EQUAL(1, do_v8__param_history[0].p0);
    ASSERT_INT_EQUAL(8, do_v8__param_history[0].p7);

    do_v8__mock_reset();
}

/*============================================================================
 *  Test: V_9 - void return, 9 params
 *==========================================================================*/

static void test_mock_v_9(void)
{
    do_v9__mock_reset();

    do_v9__mock(1, 2, 3, 4, 5, 6, 7, 8, 9);

    ASSERT_INT_EQUAL(1, do_v9__call_count);
    ASSERT_INT_EQUAL(1, do_v9__param_history[0].p0);
    ASSERT_INT_EQUAL(9, do_v9__param_history[0].p8);

    do_v9__mock_reset();
}

/*============================================================================
 *  Test: param action - mem_read (capture data from param)
 *==========================================================================*/

static void test_mock_param_action_read(void)
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
}

/*============================================================================
 *  Test: param action - mem_write (inject data into param)
 *==========================================================================*/

static void test_mock_param_action_write(void)
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
}

/*============================================================================
 *  Test: param action - multiple calls with different actions
 *==========================================================================*/

static void test_mock_param_action_multi_call(void)
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
}

/*============================================================================
 *  Test: R_3 with output parameter simulation
 *==========================================================================*/

static void test_mock_r_3_output_param(void)
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
}

/*============================================================================
 *  Test: Pointer address vs dereferenced memory capture
 *  Demonstrates the difference between:
 *  - param_history: captures the pointer VALUE (address)
 *  - mock_param_mem_read: captures the CONTENTS at that address
 *==========================================================================*/

static void test_mock_pointer_vs_memory(void)
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
}

/*============================================================================
 *  Test: Verify max calls succeeds (boundary test)
 *==========================================================================*/

static void test_mock_max_calls_void(void)
{
    size_t i;

    simple_void_func__mock_reset();

    /* Call exactly MOCK_CALL_STORAGE_MAX times - should succeed */
    for (i = 0; i < MOCK_CALL_STORAGE_MAX; i++)
    {
        simple_void_func__mock();
    }
    ASSERT_INT_EQUAL(MOCK_CALL_STORAGE_MAX, simple_void_func__call_count);

    simple_void_func__mock_reset();
}

static void test_mock_max_calls_returning(void)
{
    size_t i;

    get_value__mock_reset();

    /* Call exactly MOCK_CALL_STORAGE_MAX times - should succeed */
    for (i = 0; i < MOCK_CALL_STORAGE_MAX; i++)
    {
        (void)get_value__mock();
    }
    ASSERT_INT_EQUAL(MOCK_CALL_STORAGE_MAX, get_value__call_count);

    get_value__mock_reset();
}

/*============================================================================
 *  Test: Verify overflow aborts (subprocess test)
 *==========================================================================*/

#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

static void test_mock_overflow_aborts(void)
{
    pid_t pid;
    int status;

    pid = fork();
    if (pid == 0)
    {
        /* Child: trigger overflow and abort */
        size_t i;
        simple_void_func__mock_reset();
        for (i = 0; i <= MOCK_CALL_STORAGE_MAX; i++)
        {
            simple_void_func__mock();
        }
        /* Should not reach here */
        _exit(0);
    }

    /* Parent: wait and check child was killed by signal */
    waitpid(pid, &status, 0);

    ASSERT_TRUE(WIFSIGNALED(status));
    ASSERT_INT_EQUAL(SIGABRT, WTERMSIG(status));

}

/*============================================================================
 *  Test: Verify reset clears everything
 *==========================================================================*/

static void test_mock_reset_clears_all(void)
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
}

/*============================================================================
 *  Test: Struct-by-value parameter capture
 *==========================================================================*/

static void test_mock_struct_param(void)
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
}

/*============================================================================
 *  Test: Nested struct-by-value parameter
 *==========================================================================*/

static void test_mock_nested_struct_param(void)
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
}

/*============================================================================
 *  Test: Struct return value (no params)
 *==========================================================================*/

static void test_mock_struct_return(void)
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
}

/*============================================================================
 *  Test: Struct param and struct return
 *==========================================================================*/

static void test_mock_struct_param_and_return(void)
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
}

/*============================================================================
 *  Test: Multiple struct params
 *==========================================================================*/

static void test_mock_multiple_struct_params(void)
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
}

/*============================================================================
 *  Test: R_2_S - struct return, 2 struct params
 *==========================================================================*/

static void test_mock_r_2_s(void)
{
    struct point a = {1, 2};
    struct point b = {3, 4};
    struct point result;

    blend_points__mock_reset();

    blend_points__return_queue[0].x = 2;
    blend_points__return_queue[0].y = 3;

    result = blend_points__mock(a, b);

    ASSERT_INT_EQUAL(1, blend_points__call_count);
    ASSERT_INT_EQUAL(1, blend_points__param_history[0].p0.x);
    ASSERT_INT_EQUAL(2, blend_points__param_history[0].p0.y);
    ASSERT_INT_EQUAL(3, blend_points__param_history[0].p1.x);
    ASSERT_INT_EQUAL(4, blend_points__param_history[0].p1.y);
    ASSERT_INT_EQUAL(2, result.x);
    ASSERT_INT_EQUAL(3, result.y);

    blend_points__mock_reset();
}

/*============================================================================
 *  Test: R_3_S - struct return, 3 params (struct + primitives)
 *==========================================================================*/

static void test_mock_r_3_s(void)
{
    struct point input = {5, 10};
    struct point result;

    scale_point__mock_reset();

    scale_point__return_queue[0].x = 10;
    scale_point__return_queue[0].y = 20;

    result = scale_point__mock(input, 2, 3);

    ASSERT_INT_EQUAL(1, scale_point__call_count);
    ASSERT_INT_EQUAL(5, scale_point__param_history[0].p0.x);
    ASSERT_INT_EQUAL(10, scale_point__param_history[0].p0.y);
    ASSERT_INT_EQUAL(2, scale_point__param_history[0].p1);
    ASSERT_INT_EQUAL(3, scale_point__param_history[0].p2);
    ASSERT_INT_EQUAL(10, result.x);
    ASSERT_INT_EQUAL(20, result.y);

    scale_point__mock_reset();
}

/*============================================================================
 *  Test: R_4_S - nested struct return, 4 params
 *==========================================================================*/

static void test_mock_r_4_s(void)
{
    struct point origin = {0, 0};
    struct point size = {100, 200};
    struct rect result;

    compose_rect__mock_reset();

    compose_rect__return_queue[0].origin.x = 10;
    compose_rect__return_queue[0].origin.y = 20;
    compose_rect__return_queue[0].size.x = 50;
    compose_rect__return_queue[0].size.y = 60;

    result = compose_rect__mock(origin, size, 1, 2);

    ASSERT_INT_EQUAL(1, compose_rect__call_count);
    ASSERT_INT_EQUAL(0, compose_rect__param_history[0].p0.x);
    ASSERT_INT_EQUAL(0, compose_rect__param_history[0].p0.y);
    ASSERT_INT_EQUAL(100, compose_rect__param_history[0].p1.x);
    ASSERT_INT_EQUAL(200, compose_rect__param_history[0].p1.y);
    ASSERT_INT_EQUAL(1, compose_rect__param_history[0].p2);
    ASSERT_INT_EQUAL(2, compose_rect__param_history[0].p3);
    ASSERT_INT_EQUAL(10, result.origin.x);
    ASSERT_INT_EQUAL(20, result.origin.y);
    ASSERT_INT_EQUAL(50, result.size.x);
    ASSERT_INT_EQUAL(60, result.size.y);

    compose_rect__mock_reset();
}

/*============================================================================
 *  Test: R_5_S - returns int, 5 params with structs
 *==========================================================================*/

static void test_mock_r_5_s(void)
{
    struct point a = {1, 2};
    struct point b = {3, 4};
    int result;

    do_r5s__mock_reset();

    do_r5s__return_queue[0] = 55;

    result = do_r5s__mock(a, b, 10, 20, 30);

    ASSERT_INT_EQUAL(55, result);
    ASSERT_INT_EQUAL(1, do_r5s__call_count);
    ASSERT_INT_EQUAL(1, do_r5s__param_history[0].p0.x);
    ASSERT_INT_EQUAL(2, do_r5s__param_history[0].p0.y);
    ASSERT_INT_EQUAL(3, do_r5s__param_history[0].p1.x);
    ASSERT_INT_EQUAL(4, do_r5s__param_history[0].p1.y);
    ASSERT_INT_EQUAL(10, do_r5s__param_history[0].p2);
    ASSERT_INT_EQUAL(20, do_r5s__param_history[0].p3);
    ASSERT_INT_EQUAL(30, do_r5s__param_history[0].p4);

    do_r5s__mock_reset();
}

/*============================================================================
 *  Test: R_6_S - returns int, 6 params with structs
 *==========================================================================*/

static void test_mock_r_6_s(void)
{
    struct point a = {7, 8};
    struct point b = {9, 10};
    int result;

    do_r6s__mock_reset();

    do_r6s__return_queue[0] = 66;

    result = do_r6s__mock(a, b, 11, 22, 33, 44);

    ASSERT_INT_EQUAL(66, result);
    ASSERT_INT_EQUAL(1, do_r6s__call_count);
    ASSERT_INT_EQUAL(7, do_r6s__param_history[0].p0.x);
    ASSERT_INT_EQUAL(8, do_r6s__param_history[0].p0.y);
    ASSERT_INT_EQUAL(9, do_r6s__param_history[0].p1.x);
    ASSERT_INT_EQUAL(10, do_r6s__param_history[0].p1.y);
    ASSERT_INT_EQUAL(11, do_r6s__param_history[0].p2);
    ASSERT_INT_EQUAL(22, do_r6s__param_history[0].p3);
    ASSERT_INT_EQUAL(33, do_r6s__param_history[0].p4);
    ASSERT_INT_EQUAL(44, do_r6s__param_history[0].p5);

    do_r6s__mock_reset();
}

/*============================================================================
 *  Test: R_2_S reset clears all state
 *==========================================================================*/

static void test_mock_r_2_s_reset(void)
{
    struct point a = {1, 2};
    struct point b = {3, 4};

    blend_points__mock_reset();

    blend_points__return_queue[0].x = 99;
    blend_points__return_queue[0].y = 99;
    (void)blend_points__mock(a, b);

    ASSERT_INT_EQUAL(1, blend_points__call_count);

    blend_points__mock_reset();

    ASSERT_INT_EQUAL(0, blend_points__call_count);
    ASSERT_INT_EQUAL(0, blend_points__return_queue[0].x);
    ASSERT_INT_EQUAL(0, blend_points__param_history[0].p0.x);
}

/*============================================================================
 *  Test: R_2_S callback fires with struct params
 *==========================================================================*/

static size_t cb_r2s_last_index;
static struct point cb_r2s_p0;
static struct point cb_r2s_p1;

static void on_blend_points(size_t call_index, struct point p0, struct point p1)
{
    cb_r2s_last_index = call_index;
    cb_r2s_p0 = p0;
    cb_r2s_p1 = p1;
}

static void test_mock_r_2_s_callback(void)
{
    struct point a = {11, 22};
    struct point b = {33, 44};

    blend_points__mock_reset();
    cb_r2s_last_index = 999;

    blend_points__return_queue[0].x = 0;
    blend_points__return_queue[0].y = 0;
    blend_points__callback = on_blend_points;

    (void)blend_points__mock(a, b);

    ASSERT_INT_EQUAL(0, cb_r2s_last_index);
    ASSERT_INT_EQUAL(11, cb_r2s_p0.x);
    ASSERT_INT_EQUAL(22, cb_r2s_p0.y);
    ASSERT_INT_EQUAL(33, cb_r2s_p1.x);
    ASSERT_INT_EQUAL(44, cb_r2s_p1.y);

    blend_points__mock_reset();
}

/*============================================================================
 *  Test: R_3_S multiple calls with return queue
 *==========================================================================*/

static void test_mock_r_3_s_return_queue(void)
{
    struct point input = {1, 1};
    struct point r1, r2;

    scale_point__mock_reset();

    scale_point__return_queue[0].x = 10;
    scale_point__return_queue[0].y = 10;
    scale_point__return_queue[1].x = 20;
    scale_point__return_queue[1].y = 20;

    r1 = scale_point__mock(input, 1, 1);
    r2 = scale_point__mock(input, 2, 2);

    ASSERT_INT_EQUAL(2, scale_point__call_count);
    ASSERT_INT_EQUAL(10, r1.x);
    ASSERT_INT_EQUAL(10, r1.y);
    ASSERT_INT_EQUAL(20, r2.x);
    ASSERT_INT_EQUAL(20, r2.y);

    /* Verify both calls captured params */
    ASSERT_INT_EQUAL(1, scale_point__param_history[0].p1);
    ASSERT_INT_EQUAL(2, scale_point__param_history[1].p1);

    scale_point__mock_reset();
}

/*============================================================================
 *  Test: Multiple calls with struct returns
 *==========================================================================*/

static void test_mock_struct_return_queue(void)
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
}

/*============================================================================
 *  Test: Callback - V_V (void, no params)
 *==========================================================================*/

static size_t cb_vv_last_index;
static int cb_vv_fire_count;

static void on_simple_void(size_t call_index)
{
    cb_vv_last_index = call_index;
    cb_vv_fire_count++;
}

static void test_mock_callback_v_v(void)
{
    simple_void_func__mock_reset();
    cb_vv_fire_count = 0;
    cb_vv_last_index = 999;

    simple_void_func__callback = on_simple_void;

    simple_void_func__mock();
    ASSERT_INT_EQUAL(1, cb_vv_fire_count);
    ASSERT_INT_EQUAL(0, cb_vv_last_index);

    simple_void_func__mock();
    ASSERT_INT_EQUAL(2, cb_vv_fire_count);
    ASSERT_INT_EQUAL(1, cb_vv_last_index);

    simple_void_func__mock_reset();
}

/*============================================================================
 *  Test: Callback - R_2 (returns value, 2 params)
 *==========================================================================*/

static size_t cb_r2_last_index;
static int cb_r2_captured_p0;
static int cb_r2_captured_p1;
static int cb_r2_fire_count;

static void on_add_numbers(size_t call_index, int p0, int p1)
{
    cb_r2_last_index = call_index;
    cb_r2_captured_p0 = p0;
    cb_r2_captured_p1 = p1;
    cb_r2_fire_count++;
}

static void test_mock_callback_r_2(void)
{
    int result;

    add_numbers__mock_reset();
    cb_r2_fire_count = 0;

    add_numbers__return_queue[0] = 30;
    add_numbers__return_queue[1] = 77;
    add_numbers__callback = on_add_numbers;

    result = add_numbers__mock(10, 20);
    ASSERT_INT_EQUAL(30, result);
    ASSERT_INT_EQUAL(1, cb_r2_fire_count);
    ASSERT_INT_EQUAL(0, cb_r2_last_index);
    ASSERT_INT_EQUAL(10, cb_r2_captured_p0);
    ASSERT_INT_EQUAL(20, cb_r2_captured_p1);

    result = add_numbers__mock(33, 44);
    ASSERT_INT_EQUAL(77, result);
    ASSERT_INT_EQUAL(2, cb_r2_fire_count);
    ASSERT_INT_EQUAL(1, cb_r2_last_index);
    ASSERT_INT_EQUAL(33, cb_r2_captured_p0);
    ASSERT_INT_EQUAL(44, cb_r2_captured_p1);

    add_numbers__mock_reset();
}

/*============================================================================
 *  Test: Callback - V_1_S (struct-safe, void return, 1 struct param)
 *==========================================================================*/

static size_t cb_v1s_last_index;
static struct point cb_v1s_captured_point;

static void on_draw_point(size_t call_index, struct point pt)
{
    cb_v1s_last_index = call_index;
    cb_v1s_captured_point = pt;
}

static void test_mock_callback_v_1_s(void)
{
    struct point p = {42, 84};

    draw_point__mock_reset();
    cb_v1s_last_index = 999;

    draw_point__callback = on_draw_point;

    draw_point__mock(p);
    ASSERT_INT_EQUAL(0, cb_v1s_last_index);
    ASSERT_INT_EQUAL(42, cb_v1s_captured_point.x);
    ASSERT_INT_EQUAL(84, cb_v1s_captured_point.y);

    draw_point__mock_reset();
}

/*============================================================================
 *  Test: Callback - reset clears callback
 *==========================================================================*/

static void test_mock_callback_reset_clears(void)
{
    simple_void_func__mock_reset();
    simple_void_func__callback = on_simple_void;

    ASSERT_TRUE(simple_void_func__callback != NULL);

    simple_void_func__mock_reset();
    ASSERT_NULL(simple_void_func__callback);
}

/*============================================================================
 *  Test: Callback - NULL callback is no-op
 *==========================================================================*/

static void test_mock_callback_null_noop(void)
{
    add_numbers__mock_reset();
    cb_r2_fire_count = 0;

    /* callback is NULL after reset - should not crash */
    add_numbers__return_queue[0] = 5;
    (void)add_numbers__mock(1, 2);

    ASSERT_INT_EQUAL(0, cb_r2_fire_count);
    ASSERT_INT_EQUAL(1, add_numbers__call_count);

    add_numbers__mock_reset();
}

/*============================================================================
 *  Test: Callback fires with correct call_index matching param_history
 *==========================================================================*/

static size_t cb_indices[4];
static int cb_index_count;

static void on_set_value_index(size_t call_index, int val)
{
    (void)val;
    cb_indices[cb_index_count++] = call_index;
}

static void test_mock_callback_index_sequence(void)
{
    set_value__mock_reset();
    cb_index_count = 0;

    set_value__callback = on_set_value_index;

    set_value__mock(10);
    set_value__mock(20);
    set_value__mock(30);

    ASSERT_INT_EQUAL(3, cb_index_count);
    ASSERT_INT_EQUAL(0, cb_indices[0]);
    ASSERT_INT_EQUAL(1, cb_indices[1]);
    ASSERT_INT_EQUAL(2, cb_indices[2]);

    /* Verify param_history matches */
    ASSERT_INT_EQUAL(10, set_value__param_history[0].p0);
    ASSERT_INT_EQUAL(20, set_value__param_history[1].p0);
    ASSERT_INT_EQUAL(30, set_value__param_history[2].p0);

    set_value__mock_reset();
}

/*============================================================================
 *  Test Suite
 *==========================================================================*/

static void suite_mock_basic(void)
{
    lfg_ctest(test_mock_v_v);
    lfg_ctest(test_mock_r_v);
    lfg_ctest(test_mock_v_1);
    lfg_ctest(test_mock_r_1);
    lfg_ctest(test_mock_v_2);
    lfg_ctest(test_mock_r_2);
    lfg_ctest(test_mock_v_3);
    lfg_ctest(test_mock_r_3);
    lfg_ctest(test_mock_r_4);
    lfg_ctest(test_mock_r_5);
    lfg_ctest(test_mock_r_6);
    lfg_ctest(test_mock_r_7);
    lfg_ctest(test_mock_r_8);
    lfg_ctest(test_mock_r_9);
    lfg_ctest(test_mock_v_4);
    lfg_ctest(test_mock_v_5);
    lfg_ctest(test_mock_v_6);
    lfg_ctest(test_mock_v_7);
    lfg_ctest(test_mock_v_8);
    lfg_ctest(test_mock_v_9);
}

static void suite_mock_struct_by_value(void)
{
    lfg_ctest(test_mock_struct_param);
    lfg_ctest(test_mock_nested_struct_param);
    lfg_ctest(test_mock_struct_return);
    lfg_ctest(test_mock_struct_param_and_return);
    lfg_ctest(test_mock_multiple_struct_params);
    lfg_ctest(test_mock_r_2_s);
    lfg_ctest(test_mock_r_3_s);
    lfg_ctest(test_mock_r_4_s);
    lfg_ctest(test_mock_r_5_s);
    lfg_ctest(test_mock_r_6_s);
    lfg_ctest(test_mock_r_2_s_reset);
    lfg_ctest(test_mock_r_2_s_callback);
    lfg_ctest(test_mock_r_3_s_return_queue);
    lfg_ctest(test_mock_struct_return_queue);
}

static void suite_mock_param_actions(void)
{
    lfg_ctest(test_mock_param_action_read);
    lfg_ctest(test_mock_param_action_write);
    lfg_ctest(test_mock_param_action_multi_call);
    lfg_ctest(test_mock_r_3_output_param);
    lfg_ctest(test_mock_pointer_vs_memory);
    lfg_ctest(test_mock_reset_clears_all);
}

static void suite_mock_overflow(void)
{
    lfg_ctest(test_mock_max_calls_void);
    lfg_ctest(test_mock_max_calls_returning);
    lfg_ctest(test_mock_overflow_aborts);
}

static void suite_mock_callback(void)
{
    lfg_ctest(test_mock_callback_v_v);
    lfg_ctest(test_mock_callback_r_2);
    lfg_ctest(test_mock_callback_v_1_s);
    lfg_ctest(test_mock_callback_reset_clears);
    lfg_ctest(test_mock_callback_null_noop);
    lfg_ctest(test_mock_callback_index_sequence);
}

/*============================================================================
 *  Main
 *==========================================================================*/

int main(void)
{
    lfg_ct_start();

    printf("\n");
    printf("================================================================================\n");
    printf("                    lfg-ctest MOCK FRAMEWORK TEST SUITE\n");
    printf("================================================================================\n\n");

    printf("--- SUITE 1: Basic Mock Operations ---\n");
    lfg_ct_suite(suite_mock_basic);

    printf("\n--- SUITE 2: Struct-by-Value ---\n");
    lfg_ct_suite(suite_mock_struct_by_value);

    printf("\n--- SUITE 3: Parameter Actions ---\n");
    lfg_ct_suite(suite_mock_param_actions);

    printf("\n--- SUITE 4: Storage Limits ---\n");
    lfg_ct_suite(suite_mock_overflow);

    printf("\n--- SUITE 5: Callbacks ---\n");
    lfg_ct_suite(suite_mock_callback);

    printf("\n");
    lfg_ct_print_summary();

    return lfg_ct_return();
}
