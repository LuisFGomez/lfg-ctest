/**
 * @file
 * @brief       lfg-ctest mocking helpers.
 *
 * Macro naming convention: {R|V}_{V|N}[_S]
 *   First letter:  R = returns a value, V = void return
 *   Second letter: V = void (no params), N = number of parameters (1-9)
 *   _S suffix:     use ONLY when a parameter is a struct-by-value. Struct
 *                  return types do NOT require _S -- the plain R_N variants
 *                  handle struct returns and keep __param_actions available.
 *                  _S drops __param_actions (mem/str read/write) because the
 *                  parameter switch cannot cast struct-by-value through
 *                  (void*)(size_t). For struct-by-value params, mock_param_mem_*
 *                  would not help anyway (the mock receives a copy); read
 *                  __param_history[i].pX.field directly instead.
 * Examples:
 *   DECLARE_MOCK_R_2 = returns value, 2 params (return may be a struct)
 *   DECLARE_MOCK_V_V = void return, no params
 *   DECLARE_MOCK_R_1_S = returns value, 1 struct-by-value param
 */

#ifndef LFG_CTEST_MOCK_H_
#define LFG_CTEST_MOCK_H_

/*============================================================================
 *  Includes
 *==========================================================================*/

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#include "lfg-ctest.h"

/*============================================================================
 *  Defines/Typedefs
 *==========================================================================*/

typedef void *mock_param_action_t;

enum _mock_param_action_dir
{
    eMOCK_PARAM_ACTION_DIR_READ,
    eMOCK_PARAM_ACTION_DIR_WRITE,
    eMOCK_PARAM_ACTION_DIR_READ_STR,
    eMOCK_PARAM_ACTION_DIR_WRITE_STR,
};

struct _mock_param_action
{
    struct _mock_param_action *next; /* linked list member must be first */
    enum _mock_param_action_dir dir;
    unsigned call_index;
    unsigned parameter_index;
    void *buffer;
    size_t buf_size;
};

/** Treat parameter as memory and read bytes from it.
 * This will instruct the mocked function to:
 * - treat an incoming parameter at position @p paramidx as a pointer
 * - copy @p buf_size bytes into @p buffer (capturing the data).
 * - Do this only for @p callidx -th call of the mocked function.
 * @param[in]  action   existing action chain to append to, or NULL to start new chain
 * @param[in]  callidx  the call index (0-based) for this action
 * @param[in]  paramidx the 0-based parameter position
 * @param[out] buffer   destination to copy data into
 * @param[in]  buf_size number of bytes to copy
 * @return     the action chain (use this for subsequent calls or assign to __param_actions)
 */
mock_param_action_t mock_param_mem_read(
        mock_param_action_t action, unsigned callidx, unsigned paramidx, void *buffer, size_t buf_size);

/** Treat parameter as memory and write bytes to it.
 * This will instruct the mocked function to:
 * - treat an incoming parameter at position @p paramidx as a pointer
 * - copy @p buf_size bytes from @p buffer into the parameter (injecting data).
 * - Do this only for @p callidx -th call of the mocked function.
 * @param[in]  action   existing action chain to append to, or NULL to start new chain
 * @param[in]  callidx  the call index (0-based) for this action
 * @param[in]  paramidx the 0-based parameter position
 * @param[in]  buffer   source data to copy from
 * @param[in]  buf_size number of bytes to copy
 * @return     the action chain (use this for subsequent calls or assign to __param_actions)
 */
mock_param_action_t mock_param_mem_write(
        mock_param_action_t action, unsigned callidx, unsigned paramidx, void *buffer, size_t buf_size);

/** Treat parameter as a string and read (capture) it.
 * Like mock_param_mem_read but uses snprintf instead of memcpy,
 * so it stops at the null terminator and never reads past it.
 * @param[in]  action   existing action chain to append to, or NULL to start new chain
 * @param[in]  callidx  the call index (0-based) for this action
 * @param[in]  paramidx the 0-based parameter position
 * @param[out] buffer   destination to copy string into
 * @param[in]  buf_size size of @p buffer (includes null terminator)
 * @return     the action chain (use this for subsequent calls or assign to __param_actions)
 */
mock_param_action_t mock_param_str_read(
        mock_param_action_t action, unsigned callidx, unsigned paramidx, char *buffer, size_t buf_size);

/** Treat parameter as a string buffer and write (inject) into it.
 * Like mock_param_mem_write but uses snprintf instead of memcpy,
 * so it null-terminates and never writes past buf_size.
 * @param[in]  action   existing action chain to append to, or NULL to start new chain
 * @param[in]  callidx  the call index (0-based) for this action
 * @param[in]  paramidx the 0-based parameter position
 * @param[in]  buffer   source string to inject
 * @param[in]  buf_size size of the destination parameter buffer
 * @return     the action chain (use this for subsequent calls or assign to __param_actions)
 */
mock_param_action_t mock_param_str_write(
        mock_param_action_t action, unsigned callidx, unsigned paramidx, const char *buffer, size_t buf_size);

/** Frees all linked parameter operations.
 * @param[in] action    parameter action chain to destroy.
 */
void mock_param_destroy(mock_param_action_t action);

/** Register a mock reset function for bulk reset.
 * Called automatically by each mock on first invocation. Deduplicates entries.
 * @param[in] reset_fn   pointer to the mock's __mock_reset function.
 */
void _mock_register_reset(void (*reset_fn)(void));

/** Reset all mocks that have been invoked since the last call to mock_reset_all().
 * Iterates the auto-populated registry and calls each registered reset function,
 * then clears the registry.
 */
void mock_reset_all(void);

/** maximum number of function calls to store (override at compile time) */
#ifndef MOCK_CALL_STORAGE_MAX
#define MOCK_CALL_STORAGE_MAX 32
#endif

/** maximum number of distinct mocks that can be registered for bulk reset */
#ifndef MOCK_REGISTRY_MAX
#define MOCK_REGISTRY_MAX 64
#endif

/*============================================================================
 *  Internal Helper Macros
 *==========================================================================*/

/* Auto-register mock reset function on first invocation */
#define _MOCK_REGISTER(_func) _mock_register_reset(_func##__mock_reset);

/* Switch case generators for parameter lookup */
#define _MOCK_SWITCH_1                                                                                                 \
    case 0:                                                                                                            \
        pparam = (void *)(size_t)_p0;                                                                                  \
        break;

#define _MOCK_SWITCH_2                                                                                                 \
    _MOCK_SWITCH_1                                                                                                     \
    case 1:                                                                                                            \
        pparam = (void *)(size_t)_p1;                                                                                  \
        break;

#define _MOCK_SWITCH_3                                                                                                 \
    _MOCK_SWITCH_2                                                                                                     \
    case 2:                                                                                                            \
        pparam = (void *)(size_t)_p2;                                                                                  \
        break;

#define _MOCK_SWITCH_4                                                                                                 \
    _MOCK_SWITCH_3                                                                                                     \
    case 3:                                                                                                            \
        pparam = (void *)(size_t)_p3;                                                                                  \
        break;

#define _MOCK_SWITCH_5                                                                                                 \
    _MOCK_SWITCH_4                                                                                                     \
    case 4:                                                                                                            \
        pparam = (void *)(size_t)_p4;                                                                                  \
        break;

#define _MOCK_SWITCH_6                                                                                                 \
    _MOCK_SWITCH_5                                                                                                     \
    case 5:                                                                                                            \
        pparam = (void *)(size_t)_p5;                                                                                  \
        break;

#define _MOCK_SWITCH_7                                                                                                 \
    _MOCK_SWITCH_6                                                                                                     \
    case 6:                                                                                                            \
        pparam = (void *)(size_t)_p6;                                                                                  \
        break;

#define _MOCK_SWITCH_8                                                                                                 \
    _MOCK_SWITCH_7                                                                                                     \
    case 7:                                                                                                            \
        pparam = (void *)(size_t)_p7;                                                                                  \
        break;

#define _MOCK_SWITCH_9                                                                                                 \
    _MOCK_SWITCH_8                                                                                                     \
    case 8:                                                                                                            \
        pparam = (void *)(size_t)_p8;                                                                                  \
        break;

/* Parameter history storage */
#define _MOCK_STORE_1 p->p0 = _p0;
#define _MOCK_STORE_2 _MOCK_STORE_1 p->p1 = _p1;
#define _MOCK_STORE_3 _MOCK_STORE_2 p->p2 = _p2;
#define _MOCK_STORE_4 _MOCK_STORE_3 p->p3 = _p3;
#define _MOCK_STORE_5 _MOCK_STORE_4 p->p4 = _p4;
#define _MOCK_STORE_6 _MOCK_STORE_5 p->p5 = _p5;
#define _MOCK_STORE_7 _MOCK_STORE_6 p->p6 = _p6;
#define _MOCK_STORE_8 _MOCK_STORE_7 p->p7 = _p7;
#define _MOCK_STORE_9 _MOCK_STORE_8 p->p8 = _p8;

/* Action loop - processes param read/write actions */
#define _MOCK_ACTION_LOOP(_func, _switch)                                                                              \
    while (action)                                                                                                     \
    {                                                                                                                  \
        void *pparam = NULL;                                                                                           \
        if (action->call_index != _func##__call_count)                                                                 \
        {                                                                                                              \
            action = action->next;                                                                                     \
            continue;                                                                                                  \
        }                                                                                                              \
        switch (action->parameter_index)                                                                               \
        {                                                                                                              \
            _switch                                                                                                    \
        }                                                                                                              \
        if (eMOCK_PARAM_ACTION_DIR_READ == action->dir)                                                                \
        {                                                                                                              \
            memcpy(action->buffer, pparam, action->buf_size);                                                          \
        }                                                                                                              \
        else if (eMOCK_PARAM_ACTION_DIR_WRITE == action->dir)                                                          \
        {                                                                                                              \
            memcpy(pparam, action->buffer, action->buf_size);                                                          \
        }                                                                                                              \
        else if (eMOCK_PARAM_ACTION_DIR_READ_STR == action->dir)                                                       \
        {                                                                                                              \
            snprintf(action->buffer, action->buf_size, "%s", (const char *)pparam);                                    \
        }                                                                                                              \
        else if (eMOCK_PARAM_ACTION_DIR_WRITE_STR == action->dir)                                                      \
        {                                                                                                              \
            snprintf((char *)pparam, action->buf_size, "%s", (const char *)action->buffer);                            \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            assert(false);                                                                                             \
        }                                                                                                              \
        action = action->next;                                                                                         \
    }

/* Callback invocation helpers */
#define _MOCK_CALLBACK_V(_func)                                                                                        \
    if (_func##__callback)                                                                                             \
    {                                                                                                                  \
        _func##__callback(i);                                                                                          \
    }

#define _MOCK_CALLBACK_1(_func)                                                                                        \
    if (_func##__callback)                                                                                             \
    {                                                                                                                  \
        _func##__callback(i, _p0);                                                                                     \
    }

#define _MOCK_CALLBACK_2(_func)                                                                                        \
    if (_func##__callback)                                                                                             \
    {                                                                                                                  \
        _func##__callback(i, _p0, _p1);                                                                                \
    }

#define _MOCK_CALLBACK_3(_func)                                                                                        \
    if (_func##__callback)                                                                                             \
    {                                                                                                                  \
        _func##__callback(i, _p0, _p1, _p2);                                                                           \
    }

#define _MOCK_CALLBACK_4(_func)                                                                                        \
    if (_func##__callback)                                                                                             \
    {                                                                                                                  \
        _func##__callback(i, _p0, _p1, _p2, _p3);                                                                      \
    }

#define _MOCK_CALLBACK_5(_func)                                                                                        \
    if (_func##__callback)                                                                                             \
    {                                                                                                                  \
        _func##__callback(i, _p0, _p1, _p2, _p3, _p4);                                                                 \
    }

#define _MOCK_CALLBACK_6(_func)                                                                                        \
    if (_func##__callback)                                                                                             \
    {                                                                                                                  \
        _func##__callback(i, _p0, _p1, _p2, _p3, _p4, _p5);                                                            \
    }

#define _MOCK_CALLBACK_7(_func)                                                                                        \
    if (_func##__callback)                                                                                             \
    {                                                                                                                  \
        _func##__callback(i, _p0, _p1, _p2, _p3, _p4, _p5, _p6);                                                       \
    }

#define _MOCK_CALLBACK_8(_func)                                                                                        \
    if (_func##__callback)                                                                                             \
    {                                                                                                                  \
        _func##__callback(i, _p0, _p1, _p2, _p3, _p4, _p5, _p6, _p7);                                                  \
    }

#define _MOCK_CALLBACK_9(_func)                                                                                        \
    if (_func##__callback)                                                                                             \
    {                                                                                                                  \
        _func##__callback(i, _p0, _p1, _p2, _p3, _p4, _p5, _p6, _p7, _p8);                                             \
    }

/* Reset function for void-return mocks */
#define _MOCK_RESET_V(_func)                                                                                           \
    void _func##__mock_reset(void)                                                                                     \
    {                                                                                                                  \
        memset(_func##__param_history, 0, sizeof(_func##__param_history));                                             \
        _func##__call_count = 0;                                                                                       \
        mock_param_destroy(_func##__param_actions);                                                                    \
        _func##__param_actions = NULL;                                                                                 \
        _func##__callback = NULL;                                                                                      \
    }

/* Reset function for returning mocks */
#define _MOCK_RESET_R(_func)                                                                                           \
    void _func##__mock_reset(void)                                                                                     \
    {                                                                                                                  \
        memset(_func##__return_queue, 0, sizeof(_func##__return_queue));                                               \
        memset(_func##__param_history, 0, sizeof(_func##__param_history));                                             \
        _func##__call_count = 0;                                                                                       \
        mock_param_destroy(_func##__param_actions);                                                                    \
        _func##__param_actions = NULL;                                                                                 \
        _func##__callback = NULL;                                                                                      \
    }

/* Overflow check - aborts immediately if call limit exceeded */
#define _MOCK_OVERFLOW_CHECK(_func)                                                                                    \
    if (i >= MOCK_CALL_STORAGE_MAX)                                                                                    \
    {                                                                                                                  \
        fprintf(stderr, "MOCK OVERFLOW: %s exceeded %d calls\n", #_func, MOCK_CALL_STORAGE_MAX);                       \
        assert(0 && "mock call storage exceeded");                                                                     \
    }

#define _MOCK_OVERFLOW_CHECK_V(_func) _MOCK_OVERFLOW_CHECK(_func)
#define _MOCK_OVERFLOW_CHECK_R(_func, _rtype) _MOCK_OVERFLOW_CHECK(_func)

/*============================================================================
 *  Void Return, No Parameters (V_V)
 *==========================================================================*/

#define DECLARE_MOCK_V_V(_func)                                                                                        \
    typedef void (*_func##__callback_t)(size_t);                                                                       \
    extern _func##__callback_t _func##__callback;                                                                      \
    extern size_t _func##__call_count;                                                                                 \
    void _func##__mock(void);                                                                                          \
    void _func##__mock_reset(void)

#define DEFINE_MOCK_V_V(_func)                                                                                         \
    _func##__callback_t _func##__callback = NULL;                                                                      \
    size_t _func##__call_count = 0;                                                                                    \
    void _func##__mock(void)                                                                                           \
    {                                                                                                                  \
        size_t i = _func##__call_count;                                                                                \
        _MOCK_REGISTER(_func)                                                                                          \
        _MOCK_OVERFLOW_CHECK_V(_func)                                                                                  \
        _MOCK_CALLBACK_V(_func)                                                                                        \
        _func##__call_count++;                                                                                         \
    }                                                                                                                  \
    void _func##__mock_reset(void)                                                                                     \
    {                                                                                                                  \
        _func##__call_count = 0;                                                                                       \
        _func##__callback = NULL;                                                                                      \
    }

/*============================================================================
 *  Returns Value, No Parameters (R_V)
 *==========================================================================*/

#define DECLARE_MOCK_R_V(_func, _rtype)                                                                                \
    typedef void (*_func##__callback_t)(size_t);                                                                       \
    extern _func##__callback_t _func##__callback;                                                                      \
    extern size_t _func##__call_count;                                                                                 \
    extern _rtype _func##__return_queue[MOCK_CALL_STORAGE_MAX];                                                        \
    _rtype _func##__mock(void);                                                                                        \
    void _func##__mock_reset(void)

#define DEFINE_MOCK_R_V(_func, _rtype)                                                                                 \
    _func##__callback_t _func##__callback = NULL;                                                                      \
    size_t _func##__call_count = 0;                                                                                    \
    _rtype _func##__return_queue[MOCK_CALL_STORAGE_MAX];                                                               \
    _rtype _func##__mock(void)                                                                                         \
    {                                                                                                                  \
        _rtype ret;                                                                                                    \
        size_t i = _func##__call_count;                                                                                \
        _MOCK_REGISTER(_func)                                                                                          \
        _MOCK_OVERFLOW_CHECK_R(_func, _rtype)                                                                          \
        ret = _func##__return_queue[i];                                                                                \
        _MOCK_CALLBACK_V(_func)                                                                                        \
        _func##__call_count++;                                                                                         \
        return ret;                                                                                                    \
    }                                                                                                                  \
    void _func##__mock_reset(void)                                                                                     \
    {                                                                                                                  \
        memset(_func##__return_queue, 0, sizeof(_func##__return_queue));                                               \
        _func##__call_count = 0;                                                                                       \
        _func##__callback = NULL;                                                                                      \
    }

/*============================================================================
 *  Void Return Mocks (V_1 through V_9)
 *==========================================================================*/

#define DECLARE_MOCK_V_1(_func, _t0)                                                                                   \
    typedef struct                                                                                                     \
    {                                                                                                                  \
        _t0 p0;                                                                                                        \
    } _func##_params;                                                                                                  \
    typedef void (*_func##__callback_t)(size_t, _t0);                                                                  \
    extern _func##__callback_t _func##__callback;                                                                      \
    extern size_t _func##__call_count;                                                                                 \
    extern _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                               \
    extern mock_param_action_t _func##__param_actions;                                                                 \
    void _func##__mock(_t0);                                                                                           \
    void _func##__mock_reset(void)

#define DEFINE_MOCK_V_1(_func, _t0)                                                                                    \
    _func##__callback_t _func##__callback = NULL;                                                                      \
    size_t _func##__call_count = 0;                                                                                    \
    _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                                      \
    mock_param_action_t _func##__param_actions = NULL;                                                                 \
    void _func##__mock(_t0 _p0)                                                                                        \
    {                                                                                                                  \
        struct _mock_param_action *action = _func##__param_actions;                                                    \
        _func##_params *p;                                                                                             \
        size_t i = _func##__call_count;                                                                                \
        _MOCK_REGISTER(_func)                                                                                          \
        _MOCK_OVERFLOW_CHECK_V(_func)                                                                                  \
        p = &_func##__param_history[i];                                                                                \
        _MOCK_STORE_1                                                                                                  \
        _MOCK_ACTION_LOOP(_func, _MOCK_SWITCH_1)                                                                       \
        _MOCK_CALLBACK_1(_func)                                                                                        \
        _func##__call_count++;                                                                                         \
    }                                                                                                                  \
    _MOCK_RESET_V(_func)

#define DECLARE_MOCK_V_2(_func, _t0, _t1)                                                                              \
    typedef struct                                                                                                     \
    {                                                                                                                  \
        _t0 p0;                                                                                                        \
        _t1 p1;                                                                                                        \
    } _func##_params;                                                                                                  \
    typedef void (*_func##__callback_t)(size_t, _t0, _t1);                                                             \
    extern _func##__callback_t _func##__callback;                                                                      \
    extern size_t _func##__call_count;                                                                                 \
    extern _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                               \
    extern mock_param_action_t _func##__param_actions;                                                                 \
    void _func##__mock(_t0, _t1);                                                                                      \
    void _func##__mock_reset(void)

#define DEFINE_MOCK_V_2(_func, _t0, _t1)                                                                               \
    _func##__callback_t _func##__callback = NULL;                                                                      \
    size_t _func##__call_count = 0;                                                                                    \
    _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                                      \
    mock_param_action_t _func##__param_actions = NULL;                                                                 \
    void _func##__mock(_t0 _p0, _t1 _p1)                                                                               \
    {                                                                                                                  \
        struct _mock_param_action *action = _func##__param_actions;                                                    \
        _func##_params *p;                                                                                             \
        size_t i = _func##__call_count;                                                                                \
        _MOCK_REGISTER(_func)                                                                                          \
        _MOCK_OVERFLOW_CHECK_V(_func)                                                                                  \
        p = &_func##__param_history[i];                                                                                \
        _MOCK_STORE_2                                                                                                  \
        _MOCK_ACTION_LOOP(_func, _MOCK_SWITCH_2)                                                                       \
        _MOCK_CALLBACK_2(_func)                                                                                        \
        _func##__call_count++;                                                                                         \
    }                                                                                                                  \
    _MOCK_RESET_V(_func)

#define DECLARE_MOCK_V_3(_func, _t0, _t1, _t2)                                                                         \
    typedef struct                                                                                                     \
    {                                                                                                                  \
        _t0 p0;                                                                                                        \
        _t1 p1;                                                                                                        \
        _t2 p2;                                                                                                        \
    } _func##_params;                                                                                                  \
    typedef void (*_func##__callback_t)(size_t, _t0, _t1, _t2);                                                        \
    extern _func##__callback_t _func##__callback;                                                                      \
    extern size_t _func##__call_count;                                                                                 \
    extern _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                               \
    extern mock_param_action_t _func##__param_actions;                                                                 \
    void _func##__mock(_t0, _t1, _t2);                                                                                 \
    void _func##__mock_reset(void)

#define DEFINE_MOCK_V_3(_func, _t0, _t1, _t2)                                                                          \
    _func##__callback_t _func##__callback = NULL;                                                                      \
    size_t _func##__call_count = 0;                                                                                    \
    _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                                      \
    mock_param_action_t _func##__param_actions = NULL;                                                                 \
    void _func##__mock(_t0 _p0, _t1 _p1, _t2 _p2)                                                                      \
    {                                                                                                                  \
        struct _mock_param_action *action = _func##__param_actions;                                                    \
        _func##_params *p;                                                                                             \
        size_t i = _func##__call_count;                                                                                \
        _MOCK_REGISTER(_func)                                                                                          \
        _MOCK_OVERFLOW_CHECK_V(_func)                                                                                  \
        p = &_func##__param_history[i];                                                                                \
        _MOCK_STORE_3                                                                                                  \
        _MOCK_ACTION_LOOP(_func, _MOCK_SWITCH_3)                                                                       \
        _MOCK_CALLBACK_3(_func)                                                                                        \
        _func##__call_count++;                                                                                         \
    }                                                                                                                  \
    _MOCK_RESET_V(_func)

#define DECLARE_MOCK_V_4(_func, _t0, _t1, _t2, _t3)                                                                    \
    typedef struct                                                                                                     \
    {                                                                                                                  \
        _t0 p0;                                                                                                        \
        _t1 p1;                                                                                                        \
        _t2 p2;                                                                                                        \
        _t3 p3;                                                                                                        \
    } _func##_params;                                                                                                  \
    typedef void (*_func##__callback_t)(size_t, _t0, _t1, _t2, _t3);                                                   \
    extern _func##__callback_t _func##__callback;                                                                      \
    extern size_t _func##__call_count;                                                                                 \
    extern _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                               \
    extern mock_param_action_t _func##__param_actions;                                                                 \
    void _func##__mock(_t0, _t1, _t2, _t3);                                                                            \
    void _func##__mock_reset(void)

#define DEFINE_MOCK_V_4(_func, _t0, _t1, _t2, _t3)                                                                     \
    _func##__callback_t _func##__callback = NULL;                                                                      \
    size_t _func##__call_count = 0;                                                                                    \
    _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                                      \
    mock_param_action_t _func##__param_actions = NULL;                                                                 \
    void _func##__mock(_t0 _p0, _t1 _p1, _t2 _p2, _t3 _p3)                                                             \
    {                                                                                                                  \
        struct _mock_param_action *action = _func##__param_actions;                                                    \
        _func##_params *p;                                                                                             \
        size_t i = _func##__call_count;                                                                                \
        _MOCK_REGISTER(_func)                                                                                          \
        _MOCK_OVERFLOW_CHECK_V(_func)                                                                                  \
        p = &_func##__param_history[i];                                                                                \
        _MOCK_STORE_4                                                                                                  \
        _MOCK_ACTION_LOOP(_func, _MOCK_SWITCH_4)                                                                       \
        _MOCK_CALLBACK_4(_func)                                                                                        \
        _func##__call_count++;                                                                                         \
    }                                                                                                                  \
    _MOCK_RESET_V(_func)

#define DECLARE_MOCK_V_5(_func, _t0, _t1, _t2, _t3, _t4)                                                               \
    typedef struct                                                                                                     \
    {                                                                                                                  \
        _t0 p0;                                                                                                        \
        _t1 p1;                                                                                                        \
        _t2 p2;                                                                                                        \
        _t3 p3;                                                                                                        \
        _t4 p4;                                                                                                        \
    } _func##_params;                                                                                                  \
    typedef void (*_func##__callback_t)(size_t, _t0, _t1, _t2, _t3, _t4);                                              \
    extern _func##__callback_t _func##__callback;                                                                      \
    extern size_t _func##__call_count;                                                                                 \
    extern _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                               \
    extern mock_param_action_t _func##__param_actions;                                                                 \
    void _func##__mock(_t0, _t1, _t2, _t3, _t4);                                                                       \
    void _func##__mock_reset(void)

#define DEFINE_MOCK_V_5(_func, _t0, _t1, _t2, _t3, _t4)                                                                \
    _func##__callback_t _func##__callback = NULL;                                                                      \
    size_t _func##__call_count = 0;                                                                                    \
    _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                                      \
    mock_param_action_t _func##__param_actions = NULL;                                                                 \
    void _func##__mock(_t0 _p0, _t1 _p1, _t2 _p2, _t3 _p3, _t4 _p4)                                                    \
    {                                                                                                                  \
        struct _mock_param_action *action = _func##__param_actions;                                                    \
        _func##_params *p;                                                                                             \
        size_t i = _func##__call_count;                                                                                \
        _MOCK_REGISTER(_func)                                                                                          \
        _MOCK_OVERFLOW_CHECK_V(_func)                                                                                  \
        p = &_func##__param_history[i];                                                                                \
        _MOCK_STORE_5                                                                                                  \
        _MOCK_ACTION_LOOP(_func, _MOCK_SWITCH_5)                                                                       \
        _MOCK_CALLBACK_5(_func)                                                                                        \
        _func##__call_count++;                                                                                         \
    }                                                                                                                  \
    _MOCK_RESET_V(_func)

#define DECLARE_MOCK_V_6(_func, _t0, _t1, _t2, _t3, _t4, _t5)                                                          \
    typedef struct                                                                                                     \
    {                                                                                                                  \
        _t0 p0;                                                                                                        \
        _t1 p1;                                                                                                        \
        _t2 p2;                                                                                                        \
        _t3 p3;                                                                                                        \
        _t4 p4;                                                                                                        \
        _t5 p5;                                                                                                        \
    } _func##_params;                                                                                                  \
    typedef void (*_func##__callback_t)(size_t, _t0, _t1, _t2, _t3, _t4, _t5);                                         \
    extern _func##__callback_t _func##__callback;                                                                      \
    extern size_t _func##__call_count;                                                                                 \
    extern _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                               \
    extern mock_param_action_t _func##__param_actions;                                                                 \
    void _func##__mock(_t0, _t1, _t2, _t3, _t4, _t5);                                                                  \
    void _func##__mock_reset(void)

#define DEFINE_MOCK_V_6(_func, _t0, _t1, _t2, _t3, _t4, _t5)                                                           \
    _func##__callback_t _func##__callback = NULL;                                                                      \
    size_t _func##__call_count = 0;                                                                                    \
    _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                                      \
    mock_param_action_t _func##__param_actions = NULL;                                                                 \
    void _func##__mock(_t0 _p0, _t1 _p1, _t2 _p2, _t3 _p3, _t4 _p4, _t5 _p5)                                           \
    {                                                                                                                  \
        struct _mock_param_action *action = _func##__param_actions;                                                    \
        _func##_params *p;                                                                                             \
        size_t i = _func##__call_count;                                                                                \
        _MOCK_REGISTER(_func)                                                                                          \
        _MOCK_OVERFLOW_CHECK_V(_func)                                                                                  \
        p = &_func##__param_history[i];                                                                                \
        _MOCK_STORE_6                                                                                                  \
        _MOCK_ACTION_LOOP(_func, _MOCK_SWITCH_6)                                                                       \
        _MOCK_CALLBACK_6(_func)                                                                                        \
        _func##__call_count++;                                                                                         \
    }                                                                                                                  \
    _MOCK_RESET_V(_func)

#define DECLARE_MOCK_V_7(_func, _t0, _t1, _t2, _t3, _t4, _t5, _t6)                                                     \
    typedef struct                                                                                                     \
    {                                                                                                                  \
        _t0 p0;                                                                                                        \
        _t1 p1;                                                                                                        \
        _t2 p2;                                                                                                        \
        _t3 p3;                                                                                                        \
        _t4 p4;                                                                                                        \
        _t5 p5;                                                                                                        \
        _t6 p6;                                                                                                        \
    } _func##_params;                                                                                                  \
    typedef void (*_func##__callback_t)(size_t, _t0, _t1, _t2, _t3, _t4, _t5, _t6);                                    \
    extern _func##__callback_t _func##__callback;                                                                      \
    extern size_t _func##__call_count;                                                                                 \
    extern _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                               \
    extern mock_param_action_t _func##__param_actions;                                                                 \
    void _func##__mock(_t0, _t1, _t2, _t3, _t4, _t5, _t6);                                                             \
    void _func##__mock_reset(void)

#define DEFINE_MOCK_V_7(_func, _t0, _t1, _t2, _t3, _t4, _t5, _t6)                                                      \
    _func##__callback_t _func##__callback = NULL;                                                                      \
    size_t _func##__call_count = 0;                                                                                    \
    _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                                      \
    mock_param_action_t _func##__param_actions = NULL;                                                                 \
    void _func##__mock(_t0 _p0, _t1 _p1, _t2 _p2, _t3 _p3, _t4 _p4, _t5 _p5, _t6 _p6)                                  \
    {                                                                                                                  \
        struct _mock_param_action *action = _func##__param_actions;                                                    \
        _func##_params *p;                                                                                             \
        size_t i = _func##__call_count;                                                                                \
        _MOCK_REGISTER(_func)                                                                                          \
        _MOCK_OVERFLOW_CHECK_V(_func)                                                                                  \
        p = &_func##__param_history[i];                                                                                \
        _MOCK_STORE_7                                                                                                  \
        _MOCK_ACTION_LOOP(_func, _MOCK_SWITCH_7)                                                                       \
        _MOCK_CALLBACK_7(_func)                                                                                        \
        _func##__call_count++;                                                                                         \
    }                                                                                                                  \
    _MOCK_RESET_V(_func)

#define DECLARE_MOCK_V_8(_func, _t0, _t1, _t2, _t3, _t4, _t5, _t6, _t7)                                                \
    typedef struct                                                                                                     \
    {                                                                                                                  \
        _t0 p0;                                                                                                        \
        _t1 p1;                                                                                                        \
        _t2 p2;                                                                                                        \
        _t3 p3;                                                                                                        \
        _t4 p4;                                                                                                        \
        _t5 p5;                                                                                                        \
        _t6 p6;                                                                                                        \
        _t7 p7;                                                                                                        \
    } _func##_params;                                                                                                  \
    typedef void (*_func##__callback_t)(size_t, _t0, _t1, _t2, _t3, _t4, _t5, _t6, _t7);                               \
    extern _func##__callback_t _func##__callback;                                                                      \
    extern size_t _func##__call_count;                                                                                 \
    extern _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                               \
    extern mock_param_action_t _func##__param_actions;                                                                 \
    void _func##__mock(_t0, _t1, _t2, _t3, _t4, _t5, _t6, _t7);                                                        \
    void _func##__mock_reset(void)

#define DEFINE_MOCK_V_8(_func, _t0, _t1, _t2, _t3, _t4, _t5, _t6, _t7)                                                 \
    _func##__callback_t _func##__callback = NULL;                                                                      \
    size_t _func##__call_count = 0;                                                                                    \
    _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                                      \
    mock_param_action_t _func##__param_actions = NULL;                                                                 \
    void _func##__mock(_t0 _p0, _t1 _p1, _t2 _p2, _t3 _p3, _t4 _p4, _t5 _p5, _t6 _p6, _t7 _p7)                         \
    {                                                                                                                  \
        struct _mock_param_action *action = _func##__param_actions;                                                    \
        _func##_params *p;                                                                                             \
        size_t i = _func##__call_count;                                                                                \
        _MOCK_REGISTER(_func)                                                                                          \
        _MOCK_OVERFLOW_CHECK_V(_func)                                                                                  \
        p = &_func##__param_history[i];                                                                                \
        _MOCK_STORE_8                                                                                                  \
        _MOCK_ACTION_LOOP(_func, _MOCK_SWITCH_8)                                                                       \
        _MOCK_CALLBACK_8(_func)                                                                                        \
        _func##__call_count++;                                                                                         \
    }                                                                                                                  \
    _MOCK_RESET_V(_func)

#define DECLARE_MOCK_V_9(_func, _t0, _t1, _t2, _t3, _t4, _t5, _t6, _t7, _t8)                                           \
    typedef struct                                                                                                     \
    {                                                                                                                  \
        _t0 p0;                                                                                                        \
        _t1 p1;                                                                                                        \
        _t2 p2;                                                                                                        \
        _t3 p3;                                                                                                        \
        _t4 p4;                                                                                                        \
        _t5 p5;                                                                                                        \
        _t6 p6;                                                                                                        \
        _t7 p7;                                                                                                        \
        _t8 p8;                                                                                                        \
    } _func##_params;                                                                                                  \
    typedef void (*_func##__callback_t)(size_t, _t0, _t1, _t2, _t3, _t4, _t5, _t6, _t7, _t8);                          \
    extern _func##__callback_t _func##__callback;                                                                      \
    extern size_t _func##__call_count;                                                                                 \
    extern _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                               \
    extern mock_param_action_t _func##__param_actions;                                                                 \
    void _func##__mock(_t0, _t1, _t2, _t3, _t4, _t5, _t6, _t7, _t8);                                                   \
    void _func##__mock_reset(void)

#define DEFINE_MOCK_V_9(_func, _t0, _t1, _t2, _t3, _t4, _t5, _t6, _t7, _t8)                                            \
    _func##__callback_t _func##__callback = NULL;                                                                      \
    size_t _func##__call_count = 0;                                                                                    \
    _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                                      \
    mock_param_action_t _func##__param_actions = NULL;                                                                 \
    void _func##__mock(_t0 _p0, _t1 _p1, _t2 _p2, _t3 _p3, _t4 _p4, _t5 _p5, _t6 _p6, _t7 _p7, _t8 _p8)                \
    {                                                                                                                  \
        struct _mock_param_action *action = _func##__param_actions;                                                    \
        _func##_params *p;                                                                                             \
        size_t i = _func##__call_count;                                                                                \
        _MOCK_REGISTER(_func)                                                                                          \
        _MOCK_OVERFLOW_CHECK_V(_func)                                                                                  \
        p = &_func##__param_history[i];                                                                                \
        _MOCK_STORE_9                                                                                                  \
        _MOCK_ACTION_LOOP(_func, _MOCK_SWITCH_9)                                                                       \
        _MOCK_CALLBACK_9(_func)                                                                                        \
        _func##__call_count++;                                                                                         \
    }                                                                                                                  \
    _MOCK_RESET_V(_func)

/*============================================================================
 *  Returning Mocks (R_1 through R_9)
 *==========================================================================*/

#define DECLARE_MOCK_R_1(_func, _rtype, _t0)                                                                           \
    typedef struct                                                                                                     \
    {                                                                                                                  \
        _t0 p0;                                                                                                        \
    } _func##_params;                                                                                                  \
    typedef void (*_func##__callback_t)(size_t, _t0);                                                                  \
    extern _func##__callback_t _func##__callback;                                                                      \
    extern size_t _func##__call_count;                                                                                 \
    extern _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                               \
    extern _rtype _func##__return_queue[MOCK_CALL_STORAGE_MAX];                                                        \
    extern mock_param_action_t _func##__param_actions;                                                                 \
    _rtype _func##__mock(_t0);                                                                                         \
    void _func##__mock_reset(void)

#define DEFINE_MOCK_R_1(_func, _rtype, _t0)                                                                            \
    _func##__callback_t _func##__callback = NULL;                                                                      \
    size_t _func##__call_count = 0;                                                                                    \
    _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                                      \
    _rtype _func##__return_queue[MOCK_CALL_STORAGE_MAX];                                                               \
    mock_param_action_t _func##__param_actions = NULL;                                                                 \
    _rtype _func##__mock(_t0 _p0)                                                                                      \
    {                                                                                                                  \
        struct _mock_param_action *action = _func##__param_actions;                                                    \
        _func##_params *p;                                                                                             \
        _rtype ret;                                                                                                    \
        size_t i = _func##__call_count;                                                                                \
        _MOCK_REGISTER(_func)                                                                                          \
        _MOCK_OVERFLOW_CHECK_R(_func, _rtype)                                                                          \
        p = &_func##__param_history[i];                                                                                \
        _MOCK_STORE_1                                                                                                  \
        ret = _func##__return_queue[i];                                                                                \
        _MOCK_ACTION_LOOP(_func, _MOCK_SWITCH_1)                                                                       \
        _MOCK_CALLBACK_1(_func)                                                                                        \
        _func##__call_count++;                                                                                         \
        return ret;                                                                                                    \
    }                                                                                                                  \
    _MOCK_RESET_R(_func)

#define DECLARE_MOCK_R_2(_func, _rtype, _t0, _t1)                                                                      \
    typedef struct                                                                                                     \
    {                                                                                                                  \
        _t0 p0;                                                                                                        \
        _t1 p1;                                                                                                        \
    } _func##_params;                                                                                                  \
    typedef void (*_func##__callback_t)(size_t, _t0, _t1);                                                             \
    extern _func##__callback_t _func##__callback;                                                                      \
    extern size_t _func##__call_count;                                                                                 \
    extern _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                               \
    extern _rtype _func##__return_queue[MOCK_CALL_STORAGE_MAX];                                                        \
    extern mock_param_action_t _func##__param_actions;                                                                 \
    _rtype _func##__mock(_t0, _t1);                                                                                    \
    void _func##__mock_reset(void)

#define DEFINE_MOCK_R_2(_func, _rtype, _t0, _t1)                                                                       \
    _func##__callback_t _func##__callback = NULL;                                                                      \
    size_t _func##__call_count = 0;                                                                                    \
    _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                                      \
    _rtype _func##__return_queue[MOCK_CALL_STORAGE_MAX];                                                               \
    mock_param_action_t _func##__param_actions = NULL;                                                                 \
    _rtype _func##__mock(_t0 _p0, _t1 _p1)                                                                             \
    {                                                                                                                  \
        struct _mock_param_action *action = _func##__param_actions;                                                    \
        _func##_params *p;                                                                                             \
        _rtype ret;                                                                                                    \
        size_t i = _func##__call_count;                                                                                \
        _MOCK_REGISTER(_func)                                                                                          \
        _MOCK_OVERFLOW_CHECK_R(_func, _rtype)                                                                          \
        p = &_func##__param_history[i];                                                                                \
        _MOCK_STORE_2                                                                                                  \
        ret = _func##__return_queue[i];                                                                                \
        _MOCK_ACTION_LOOP(_func, _MOCK_SWITCH_2)                                                                       \
        _MOCK_CALLBACK_2(_func)                                                                                        \
        _func##__call_count++;                                                                                         \
        return ret;                                                                                                    \
    }                                                                                                                  \
    _MOCK_RESET_R(_func)

#define DECLARE_MOCK_R_3(_func, _rtype, _t0, _t1, _t2)                                                                 \
    typedef struct                                                                                                     \
    {                                                                                                                  \
        _t0 p0;                                                                                                        \
        _t1 p1;                                                                                                        \
        _t2 p2;                                                                                                        \
    } _func##_params;                                                                                                  \
    typedef void (*_func##__callback_t)(size_t, _t0, _t1, _t2);                                                        \
    extern _func##__callback_t _func##__callback;                                                                      \
    extern size_t _func##__call_count;                                                                                 \
    extern _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                               \
    extern _rtype _func##__return_queue[MOCK_CALL_STORAGE_MAX];                                                        \
    extern mock_param_action_t _func##__param_actions;                                                                 \
    _rtype _func##__mock(_t0, _t1, _t2);                                                                               \
    void _func##__mock_reset(void)

#define DEFINE_MOCK_R_3(_func, _rtype, _t0, _t1, _t2)                                                                  \
    _func##__callback_t _func##__callback = NULL;                                                                      \
    size_t _func##__call_count = 0;                                                                                    \
    _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                                      \
    _rtype _func##__return_queue[MOCK_CALL_STORAGE_MAX];                                                               \
    mock_param_action_t _func##__param_actions = NULL;                                                                 \
    _rtype _func##__mock(_t0 _p0, _t1 _p1, _t2 _p2)                                                                    \
    {                                                                                                                  \
        struct _mock_param_action *action = _func##__param_actions;                                                    \
        _func##_params *p;                                                                                             \
        _rtype ret;                                                                                                    \
        size_t i = _func##__call_count;                                                                                \
        _MOCK_REGISTER(_func)                                                                                          \
        _MOCK_OVERFLOW_CHECK_R(_func, _rtype)                                                                          \
        p = &_func##__param_history[i];                                                                                \
        _MOCK_STORE_3                                                                                                  \
        ret = _func##__return_queue[i];                                                                                \
        _MOCK_ACTION_LOOP(_func, _MOCK_SWITCH_3)                                                                       \
        _MOCK_CALLBACK_3(_func)                                                                                        \
        _func##__call_count++;                                                                                         \
        return ret;                                                                                                    \
    }                                                                                                                  \
    _MOCK_RESET_R(_func)

#define DECLARE_MOCK_R_4(_func, _rtype, _t0, _t1, _t2, _t3)                                                            \
    typedef struct                                                                                                     \
    {                                                                                                                  \
        _t0 p0;                                                                                                        \
        _t1 p1;                                                                                                        \
        _t2 p2;                                                                                                        \
        _t3 p3;                                                                                                        \
    } _func##_params;                                                                                                  \
    typedef void (*_func##__callback_t)(size_t, _t0, _t1, _t2, _t3);                                                   \
    extern _func##__callback_t _func##__callback;                                                                      \
    extern size_t _func##__call_count;                                                                                 \
    extern _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                               \
    extern _rtype _func##__return_queue[MOCK_CALL_STORAGE_MAX];                                                        \
    extern mock_param_action_t _func##__param_actions;                                                                 \
    _rtype _func##__mock(_t0, _t1, _t2, _t3);                                                                          \
    void _func##__mock_reset(void)

#define DEFINE_MOCK_R_4(_func, _rtype, _t0, _t1, _t2, _t3)                                                             \
    _func##__callback_t _func##__callback = NULL;                                                                      \
    size_t _func##__call_count = 0;                                                                                    \
    _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                                      \
    _rtype _func##__return_queue[MOCK_CALL_STORAGE_MAX];                                                               \
    mock_param_action_t _func##__param_actions = NULL;                                                                 \
    _rtype _func##__mock(_t0 _p0, _t1 _p1, _t2 _p2, _t3 _p3)                                                           \
    {                                                                                                                  \
        struct _mock_param_action *action = _func##__param_actions;                                                    \
        _func##_params *p;                                                                                             \
        _rtype ret;                                                                                                    \
        size_t i = _func##__call_count;                                                                                \
        _MOCK_REGISTER(_func)                                                                                          \
        _MOCK_OVERFLOW_CHECK_R(_func, _rtype)                                                                          \
        p = &_func##__param_history[i];                                                                                \
        _MOCK_STORE_4                                                                                                  \
        ret = _func##__return_queue[i];                                                                                \
        _MOCK_ACTION_LOOP(_func, _MOCK_SWITCH_4)                                                                       \
        _MOCK_CALLBACK_4(_func)                                                                                        \
        _func##__call_count++;                                                                                         \
        return ret;                                                                                                    \
    }                                                                                                                  \
    _MOCK_RESET_R(_func)

#define DECLARE_MOCK_R_5(_func, _rtype, _t0, _t1, _t2, _t3, _t4)                                                       \
    typedef struct                                                                                                     \
    {                                                                                                                  \
        _t0 p0;                                                                                                        \
        _t1 p1;                                                                                                        \
        _t2 p2;                                                                                                        \
        _t3 p3;                                                                                                        \
        _t4 p4;                                                                                                        \
    } _func##_params;                                                                                                  \
    typedef void (*_func##__callback_t)(size_t, _t0, _t1, _t2, _t3, _t4);                                              \
    extern _func##__callback_t _func##__callback;                                                                      \
    extern size_t _func##__call_count;                                                                                 \
    extern _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                               \
    extern _rtype _func##__return_queue[MOCK_CALL_STORAGE_MAX];                                                        \
    extern mock_param_action_t _func##__param_actions;                                                                 \
    _rtype _func##__mock(_t0, _t1, _t2, _t3, _t4);                                                                     \
    void _func##__mock_reset(void)

#define DEFINE_MOCK_R_5(_func, _rtype, _t0, _t1, _t2, _t3, _t4)                                                        \
    _func##__callback_t _func##__callback = NULL;                                                                      \
    size_t _func##__call_count = 0;                                                                                    \
    _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                                      \
    _rtype _func##__return_queue[MOCK_CALL_STORAGE_MAX];                                                               \
    mock_param_action_t _func##__param_actions = NULL;                                                                 \
    _rtype _func##__mock(_t0 _p0, _t1 _p1, _t2 _p2, _t3 _p3, _t4 _p4)                                                  \
    {                                                                                                                  \
        struct _mock_param_action *action = _func##__param_actions;                                                    \
        _func##_params *p;                                                                                             \
        _rtype ret;                                                                                                    \
        size_t i = _func##__call_count;                                                                                \
        _MOCK_REGISTER(_func)                                                                                          \
        _MOCK_OVERFLOW_CHECK_R(_func, _rtype)                                                                          \
        p = &_func##__param_history[i];                                                                                \
        _MOCK_STORE_5                                                                                                  \
        ret = _func##__return_queue[i];                                                                                \
        _MOCK_ACTION_LOOP(_func, _MOCK_SWITCH_5)                                                                       \
        _MOCK_CALLBACK_5(_func)                                                                                        \
        _func##__call_count++;                                                                                         \
        return ret;                                                                                                    \
    }                                                                                                                  \
    _MOCK_RESET_R(_func)

#define DECLARE_MOCK_R_6(_func, _rtype, _t0, _t1, _t2, _t3, _t4, _t5)                                                  \
    typedef struct                                                                                                     \
    {                                                                                                                  \
        _t0 p0;                                                                                                        \
        _t1 p1;                                                                                                        \
        _t2 p2;                                                                                                        \
        _t3 p3;                                                                                                        \
        _t4 p4;                                                                                                        \
        _t5 p5;                                                                                                        \
    } _func##_params;                                                                                                  \
    typedef void (*_func##__callback_t)(size_t, _t0, _t1, _t2, _t3, _t4, _t5);                                         \
    extern _func##__callback_t _func##__callback;                                                                      \
    extern size_t _func##__call_count;                                                                                 \
    extern _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                               \
    extern _rtype _func##__return_queue[MOCK_CALL_STORAGE_MAX];                                                        \
    extern mock_param_action_t _func##__param_actions;                                                                 \
    _rtype _func##__mock(_t0, _t1, _t2, _t3, _t4, _t5);                                                                \
    void _func##__mock_reset(void)

#define DEFINE_MOCK_R_6(_func, _rtype, _t0, _t1, _t2, _t3, _t4, _t5)                                                   \
    _func##__callback_t _func##__callback = NULL;                                                                      \
    size_t _func##__call_count = 0;                                                                                    \
    _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                                      \
    _rtype _func##__return_queue[MOCK_CALL_STORAGE_MAX];                                                               \
    mock_param_action_t _func##__param_actions = NULL;                                                                 \
    _rtype _func##__mock(_t0 _p0, _t1 _p1, _t2 _p2, _t3 _p3, _t4 _p4, _t5 _p5)                                         \
    {                                                                                                                  \
        struct _mock_param_action *action = _func##__param_actions;                                                    \
        _func##_params *p;                                                                                             \
        _rtype ret;                                                                                                    \
        size_t i = _func##__call_count;                                                                                \
        _MOCK_REGISTER(_func)                                                                                          \
        _MOCK_OVERFLOW_CHECK_R(_func, _rtype)                                                                          \
        p = &_func##__param_history[i];                                                                                \
        _MOCK_STORE_6                                                                                                  \
        ret = _func##__return_queue[i];                                                                                \
        _MOCK_ACTION_LOOP(_func, _MOCK_SWITCH_6)                                                                       \
        _MOCK_CALLBACK_6(_func)                                                                                        \
        _func##__call_count++;                                                                                         \
        return ret;                                                                                                    \
    }                                                                                                                  \
    _MOCK_RESET_R(_func)

#define DECLARE_MOCK_R_7(_func, _rtype, _t0, _t1, _t2, _t3, _t4, _t5, _t6)                                             \
    typedef struct                                                                                                     \
    {                                                                                                                  \
        _t0 p0;                                                                                                        \
        _t1 p1;                                                                                                        \
        _t2 p2;                                                                                                        \
        _t3 p3;                                                                                                        \
        _t4 p4;                                                                                                        \
        _t5 p5;                                                                                                        \
        _t6 p6;                                                                                                        \
    } _func##_params;                                                                                                  \
    typedef void (*_func##__callback_t)(size_t, _t0, _t1, _t2, _t3, _t4, _t5, _t6);                                    \
    extern _func##__callback_t _func##__callback;                                                                      \
    extern size_t _func##__call_count;                                                                                 \
    extern _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                               \
    extern _rtype _func##__return_queue[MOCK_CALL_STORAGE_MAX];                                                        \
    extern mock_param_action_t _func##__param_actions;                                                                 \
    _rtype _func##__mock(_t0, _t1, _t2, _t3, _t4, _t5, _t6);                                                           \
    void _func##__mock_reset(void)

#define DEFINE_MOCK_R_7(_func, _rtype, _t0, _t1, _t2, _t3, _t4, _t5, _t6)                                              \
    _func##__callback_t _func##__callback = NULL;                                                                      \
    size_t _func##__call_count = 0;                                                                                    \
    _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                                      \
    _rtype _func##__return_queue[MOCK_CALL_STORAGE_MAX];                                                               \
    mock_param_action_t _func##__param_actions = NULL;                                                                 \
    _rtype _func##__mock(_t0 _p0, _t1 _p1, _t2 _p2, _t3 _p3, _t4 _p4, _t5 _p5, _t6 _p6)                                \
    {                                                                                                                  \
        struct _mock_param_action *action = _func##__param_actions;                                                    \
        _func##_params *p;                                                                                             \
        _rtype ret;                                                                                                    \
        size_t i = _func##__call_count;                                                                                \
        _MOCK_REGISTER(_func)                                                                                          \
        _MOCK_OVERFLOW_CHECK_R(_func, _rtype)                                                                          \
        p = &_func##__param_history[i];                                                                                \
        _MOCK_STORE_7                                                                                                  \
        ret = _func##__return_queue[i];                                                                                \
        _MOCK_ACTION_LOOP(_func, _MOCK_SWITCH_7)                                                                       \
        _MOCK_CALLBACK_7(_func)                                                                                        \
        _func##__call_count++;                                                                                         \
        return ret;                                                                                                    \
    }                                                                                                                  \
    _MOCK_RESET_R(_func)

#define DECLARE_MOCK_R_8(_func, _rtype, _t0, _t1, _t2, _t3, _t4, _t5, _t6, _t7)                                        \
    typedef struct                                                                                                     \
    {                                                                                                                  \
        _t0 p0;                                                                                                        \
        _t1 p1;                                                                                                        \
        _t2 p2;                                                                                                        \
        _t3 p3;                                                                                                        \
        _t4 p4;                                                                                                        \
        _t5 p5;                                                                                                        \
        _t6 p6;                                                                                                        \
        _t7 p7;                                                                                                        \
    } _func##_params;                                                                                                  \
    typedef void (*_func##__callback_t)(size_t, _t0, _t1, _t2, _t3, _t4, _t5, _t6, _t7);                               \
    extern _func##__callback_t _func##__callback;                                                                      \
    extern size_t _func##__call_count;                                                                                 \
    extern _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                               \
    extern _rtype _func##__return_queue[MOCK_CALL_STORAGE_MAX];                                                        \
    extern mock_param_action_t _func##__param_actions;                                                                 \
    _rtype _func##__mock(_t0, _t1, _t2, _t3, _t4, _t5, _t6, _t7);                                                      \
    void _func##__mock_reset(void)

#define DEFINE_MOCK_R_8(_func, _rtype, _t0, _t1, _t2, _t3, _t4, _t5, _t6, _t7)                                         \
    _func##__callback_t _func##__callback = NULL;                                                                      \
    size_t _func##__call_count = 0;                                                                                    \
    _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                                      \
    _rtype _func##__return_queue[MOCK_CALL_STORAGE_MAX];                                                               \
    mock_param_action_t _func##__param_actions = NULL;                                                                 \
    _rtype _func##__mock(_t0 _p0, _t1 _p1, _t2 _p2, _t3 _p3, _t4 _p4, _t5 _p5, _t6 _p6, _t7 _p7)                       \
    {                                                                                                                  \
        struct _mock_param_action *action = _func##__param_actions;                                                    \
        _func##_params *p;                                                                                             \
        _rtype ret;                                                                                                    \
        size_t i = _func##__call_count;                                                                                \
        _MOCK_REGISTER(_func)                                                                                          \
        _MOCK_OVERFLOW_CHECK_R(_func, _rtype)                                                                          \
        p = &_func##__param_history[i];                                                                                \
        _MOCK_STORE_8                                                                                                  \
        ret = _func##__return_queue[i];                                                                                \
        _MOCK_ACTION_LOOP(_func, _MOCK_SWITCH_8)                                                                       \
        _MOCK_CALLBACK_8(_func)                                                                                        \
        _func##__call_count++;                                                                                         \
        return ret;                                                                                                    \
    }                                                                                                                  \
    _MOCK_RESET_R(_func)

#define DECLARE_MOCK_R_9(_func, _rtype, _t0, _t1, _t2, _t3, _t4, _t5, _t6, _t7, _t8)                                   \
    typedef struct                                                                                                     \
    {                                                                                                                  \
        _t0 p0;                                                                                                        \
        _t1 p1;                                                                                                        \
        _t2 p2;                                                                                                        \
        _t3 p3;                                                                                                        \
        _t4 p4;                                                                                                        \
        _t5 p5;                                                                                                        \
        _t6 p6;                                                                                                        \
        _t7 p7;                                                                                                        \
        _t8 p8;                                                                                                        \
    } _func##_params;                                                                                                  \
    typedef void (*_func##__callback_t)(size_t, _t0, _t1, _t2, _t3, _t4, _t5, _t6, _t7, _t8);                          \
    extern _func##__callback_t _func##__callback;                                                                      \
    extern size_t _func##__call_count;                                                                                 \
    extern _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                               \
    extern _rtype _func##__return_queue[MOCK_CALL_STORAGE_MAX];                                                        \
    extern mock_param_action_t _func##__param_actions;                                                                 \
    _rtype _func##__mock(_t0, _t1, _t2, _t3, _t4, _t5, _t6, _t7, _t8);                                                 \
    void _func##__mock_reset(void)

#define DEFINE_MOCK_R_9(_func, _rtype, _t0, _t1, _t2, _t3, _t4, _t5, _t6, _t7, _t8)                                    \
    _func##__callback_t _func##__callback = NULL;                                                                      \
    size_t _func##__call_count = 0;                                                                                    \
    _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                                      \
    _rtype _func##__return_queue[MOCK_CALL_STORAGE_MAX];                                                               \
    mock_param_action_t _func##__param_actions = NULL;                                                                 \
    _rtype _func##__mock(_t0 _p0, _t1 _p1, _t2 _p2, _t3 _p3, _t4 _p4, _t5 _p5, _t6 _p6, _t7 _p7, _t8 _p8)              \
    {                                                                                                                  \
        struct _mock_param_action *action = _func##__param_actions;                                                    \
        _func##_params *p;                                                                                             \
        _rtype ret;                                                                                                    \
        size_t i = _func##__call_count;                                                                                \
        _MOCK_REGISTER(_func)                                                                                          \
        _MOCK_OVERFLOW_CHECK_R(_func, _rtype)                                                                          \
        p = &_func##__param_history[i];                                                                                \
        _MOCK_STORE_9                                                                                                  \
        ret = _func##__return_queue[i];                                                                                \
        _MOCK_ACTION_LOOP(_func, _MOCK_SWITCH_9)                                                                       \
        _MOCK_CALLBACK_9(_func)                                                                                        \
        _func##__call_count++;                                                                                         \
        return ret;                                                                                                    \
    }                                                                                                                  \
    _MOCK_RESET_R(_func)

/*============================================================================
 *  Simple Mocks (no param actions) - for struct-by-value support
 *
 *  These macros work with any parameter type including structs.
 *  They don't support mock_param_mem_read/write actions.
 *  Use these when:
 *  - Parameters are structs passed by value
 *  - You only need param_history and return_queue
 *==========================================================================*/

/* Simple reset - no param actions to destroy */
#define _MOCK_RESET_V_SIMPLE(_func)                                                                                    \
    void _func##__mock_reset(void)                                                                                     \
    {                                                                                                                  \
        memset(_func##__param_history, 0, sizeof(_func##__param_history));                                             \
        _func##__call_count = 0;                                                                                       \
        _func##__callback = NULL;                                                                                      \
    }

#define _MOCK_RESET_R_SIMPLE(_func)                                                                                    \
    void _func##__mock_reset(void)                                                                                     \
    {                                                                                                                  \
        memset(_func##__return_queue, 0, sizeof(_func##__return_queue));                                               \
        memset(_func##__param_history, 0, sizeof(_func##__param_history));                                             \
        _func##__call_count = 0;                                                                                       \
        _func##__callback = NULL;                                                                                      \
    }

/* V_1_S: void return, 1 param, struct-safe */
#define DECLARE_MOCK_V_1_S(_func, _t0)                                                                                 \
    typedef struct                                                                                                     \
    {                                                                                                                  \
        _t0 p0;                                                                                                        \
    } _func##_params;                                                                                                  \
    typedef void (*_func##__callback_t)(size_t, _t0);                                                                  \
    extern _func##__callback_t _func##__callback;                                                                      \
    extern size_t _func##__call_count;                                                                                 \
    extern _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                               \
    void _func##__mock(_t0);                                                                                           \
    void _func##__mock_reset(void)

#define DEFINE_MOCK_V_1_S(_func, _t0)                                                                                  \
    _func##__callback_t _func##__callback = NULL;                                                                      \
    size_t _func##__call_count = 0;                                                                                    \
    _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                                      \
    void _func##__mock(_t0 _p0)                                                                                        \
    {                                                                                                                  \
        _func##_params *p;                                                                                             \
        size_t i = _func##__call_count;                                                                                \
        _MOCK_REGISTER(_func)                                                                                          \
        _MOCK_OVERFLOW_CHECK_V(_func)                                                                                  \
        p = &_func##__param_history[i];                                                                                \
        _MOCK_STORE_1                                                                                                  \
        _MOCK_CALLBACK_1(_func)                                                                                        \
        _func##__call_count++;                                                                                         \
    }                                                                                                                  \
    _MOCK_RESET_V_SIMPLE(_func)

/* V_2_S: void return, 2 params, struct-safe */
#define DECLARE_MOCK_V_2_S(_func, _t0, _t1)                                                                            \
    typedef struct                                                                                                     \
    {                                                                                                                  \
        _t0 p0;                                                                                                        \
        _t1 p1;                                                                                                        \
    } _func##_params;                                                                                                  \
    typedef void (*_func##__callback_t)(size_t, _t0, _t1);                                                             \
    extern _func##__callback_t _func##__callback;                                                                      \
    extern size_t _func##__call_count;                                                                                 \
    extern _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                               \
    void _func##__mock(_t0, _t1);                                                                                      \
    void _func##__mock_reset(void)

#define DEFINE_MOCK_V_2_S(_func, _t0, _t1)                                                                             \
    _func##__callback_t _func##__callback = NULL;                                                                      \
    size_t _func##__call_count = 0;                                                                                    \
    _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                                      \
    void _func##__mock(_t0 _p0, _t1 _p1)                                                                               \
    {                                                                                                                  \
        _func##_params *p;                                                                                             \
        size_t i = _func##__call_count;                                                                                \
        _MOCK_REGISTER(_func)                                                                                          \
        _MOCK_OVERFLOW_CHECK_V(_func)                                                                                  \
        p = &_func##__param_history[i];                                                                                \
        _MOCK_STORE_2                                                                                                  \
        _MOCK_CALLBACK_2(_func)                                                                                        \
        _func##__call_count++;                                                                                         \
    }                                                                                                                  \
    _MOCK_RESET_V_SIMPLE(_func)

/* V_3_S: void return, 3 params, struct-safe */
#define DECLARE_MOCK_V_3_S(_func, _t0, _t1, _t2)                                                                       \
    typedef struct                                                                                                     \
    {                                                                                                                  \
        _t0 p0;                                                                                                        \
        _t1 p1;                                                                                                        \
        _t2 p2;                                                                                                        \
    } _func##_params;                                                                                                  \
    typedef void (*_func##__callback_t)(size_t, _t0, _t1, _t2);                                                        \
    extern _func##__callback_t _func##__callback;                                                                      \
    extern size_t _func##__call_count;                                                                                 \
    extern _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                               \
    void _func##__mock(_t0, _t1, _t2);                                                                                 \
    void _func##__mock_reset(void)

#define DEFINE_MOCK_V_3_S(_func, _t0, _t1, _t2)                                                                        \
    _func##__callback_t _func##__callback = NULL;                                                                      \
    size_t _func##__call_count = 0;                                                                                    \
    _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                                      \
    void _func##__mock(_t0 _p0, _t1 _p1, _t2 _p2)                                                                      \
    {                                                                                                                  \
        _func##_params *p;                                                                                             \
        size_t i = _func##__call_count;                                                                                \
        _MOCK_REGISTER(_func)                                                                                          \
        _MOCK_OVERFLOW_CHECK_V(_func)                                                                                  \
        p = &_func##__param_history[i];                                                                                \
        _MOCK_STORE_3                                                                                                  \
        _MOCK_CALLBACK_3(_func)                                                                                        \
        _func##__call_count++;                                                                                         \
    }                                                                                                                  \
    _MOCK_RESET_V_SIMPLE(_func)

/* R_V_S: returns value, no params, struct-safe */
#define DECLARE_MOCK_R_V_S(_func, _rtype)                                                                              \
    typedef void (*_func##__callback_t)(size_t);                                                                       \
    extern _func##__callback_t _func##__callback;                                                                      \
    extern size_t _func##__call_count;                                                                                 \
    extern _rtype _func##__return_queue[MOCK_CALL_STORAGE_MAX];                                                        \
    _rtype _func##__mock(void);                                                                                        \
    void _func##__mock_reset(void)

#define DEFINE_MOCK_R_V_S(_func, _rtype)                                                                               \
    _func##__callback_t _func##__callback = NULL;                                                                      \
    size_t _func##__call_count = 0;                                                                                    \
    _rtype _func##__return_queue[MOCK_CALL_STORAGE_MAX];                                                               \
    _rtype _func##__mock(void)                                                                                         \
    {                                                                                                                  \
        _rtype ret;                                                                                                    \
        size_t i = _func##__call_count;                                                                                \
        _MOCK_REGISTER(_func)                                                                                          \
        _MOCK_OVERFLOW_CHECK_R(_func, _rtype)                                                                          \
        ret = _func##__return_queue[i];                                                                                \
        _MOCK_CALLBACK_V(_func)                                                                                        \
        _func##__call_count++;                                                                                         \
        return ret;                                                                                                    \
    }                                                                                                                  \
    void _func##__mock_reset(void)                                                                                     \
    {                                                                                                                  \
        memset(_func##__return_queue, 0, sizeof(_func##__return_queue));                                               \
        _func##__call_count = 0;                                                                                       \
        _func##__callback = NULL;                                                                                      \
    }

/* R_1_S: returns value, 1 param, struct-safe */
#define DECLARE_MOCK_R_1_S(_func, _rtype, _t0)                                                                         \
    typedef struct                                                                                                     \
    {                                                                                                                  \
        _t0 p0;                                                                                                        \
    } _func##_params;                                                                                                  \
    typedef void (*_func##__callback_t)(size_t, _t0);                                                                  \
    extern _func##__callback_t _func##__callback;                                                                      \
    extern size_t _func##__call_count;                                                                                 \
    extern _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                               \
    extern _rtype _func##__return_queue[MOCK_CALL_STORAGE_MAX];                                                        \
    _rtype _func##__mock(_t0);                                                                                         \
    void _func##__mock_reset(void)

#define DEFINE_MOCK_R_1_S(_func, _rtype, _t0)                                                                          \
    _func##__callback_t _func##__callback = NULL;                                                                      \
    size_t _func##__call_count = 0;                                                                                    \
    _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                                      \
    _rtype _func##__return_queue[MOCK_CALL_STORAGE_MAX];                                                               \
    _rtype _func##__mock(_t0 _p0)                                                                                      \
    {                                                                                                                  \
        _func##_params *p;                                                                                             \
        _rtype ret;                                                                                                    \
        size_t i = _func##__call_count;                                                                                \
        _MOCK_REGISTER(_func)                                                                                          \
        _MOCK_OVERFLOW_CHECK_R(_func, _rtype)                                                                          \
        p = &_func##__param_history[i];                                                                                \
        _MOCK_STORE_1                                                                                                  \
        ret = _func##__return_queue[i];                                                                                \
        _MOCK_CALLBACK_1(_func)                                                                                        \
        _func##__call_count++;                                                                                         \
        return ret;                                                                                                    \
    }                                                                                                                  \
    _MOCK_RESET_R_SIMPLE(_func)

/* R_2_S: returns value, 2 params, struct-safe */
#define DECLARE_MOCK_R_2_S(_func, _rtype, _t0, _t1)                                                                    \
    typedef struct                                                                                                     \
    {                                                                                                                  \
        _t0 p0;                                                                                                        \
        _t1 p1;                                                                                                        \
    } _func##_params;                                                                                                  \
    typedef void (*_func##__callback_t)(size_t, _t0, _t1);                                                             \
    extern _func##__callback_t _func##__callback;                                                                      \
    extern size_t _func##__call_count;                                                                                 \
    extern _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                               \
    extern _rtype _func##__return_queue[MOCK_CALL_STORAGE_MAX];                                                        \
    _rtype _func##__mock(_t0, _t1);                                                                                    \
    void _func##__mock_reset(void)

#define DEFINE_MOCK_R_2_S(_func, _rtype, _t0, _t1)                                                                     \
    _func##__callback_t _func##__callback = NULL;                                                                      \
    size_t _func##__call_count = 0;                                                                                    \
    _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                                      \
    _rtype _func##__return_queue[MOCK_CALL_STORAGE_MAX];                                                               \
    _rtype _func##__mock(_t0 _p0, _t1 _p1)                                                                             \
    {                                                                                                                  \
        _func##_params *p;                                                                                             \
        _rtype ret;                                                                                                    \
        size_t i = _func##__call_count;                                                                                \
        _MOCK_REGISTER(_func)                                                                                          \
        _MOCK_OVERFLOW_CHECK_R(_func, _rtype)                                                                          \
        p = &_func##__param_history[i];                                                                                \
        _MOCK_STORE_2                                                                                                  \
        ret = _func##__return_queue[i];                                                                                \
        _MOCK_CALLBACK_2(_func)                                                                                        \
        _func##__call_count++;                                                                                         \
        return ret;                                                                                                    \
    }                                                                                                                  \
    _MOCK_RESET_R_SIMPLE(_func)

/* R_3_S: returns value, 3 params, struct-safe */
#define DECLARE_MOCK_R_3_S(_func, _rtype, _t0, _t1, _t2)                                                               \
    typedef struct                                                                                                     \
    {                                                                                                                  \
        _t0 p0;                                                                                                        \
        _t1 p1;                                                                                                        \
        _t2 p2;                                                                                                        \
    } _func##_params;                                                                                                  \
    typedef void (*_func##__callback_t)(size_t, _t0, _t1, _t2);                                                        \
    extern _func##__callback_t _func##__callback;                                                                      \
    extern size_t _func##__call_count;                                                                                 \
    extern _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                               \
    extern _rtype _func##__return_queue[MOCK_CALL_STORAGE_MAX];                                                        \
    _rtype _func##__mock(_t0, _t1, _t2);                                                                               \
    void _func##__mock_reset(void)

#define DEFINE_MOCK_R_3_S(_func, _rtype, _t0, _t1, _t2)                                                                \
    _func##__callback_t _func##__callback = NULL;                                                                      \
    size_t _func##__call_count = 0;                                                                                    \
    _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                                      \
    _rtype _func##__return_queue[MOCK_CALL_STORAGE_MAX];                                                               \
    _rtype _func##__mock(_t0 _p0, _t1 _p1, _t2 _p2)                                                                    \
    {                                                                                                                  \
        _func##_params *p;                                                                                             \
        _rtype ret;                                                                                                    \
        size_t i = _func##__call_count;                                                                                \
        _MOCK_REGISTER(_func)                                                                                          \
        _MOCK_OVERFLOW_CHECK_R(_func, _rtype)                                                                          \
        p = &_func##__param_history[i];                                                                                \
        _MOCK_STORE_3                                                                                                  \
        ret = _func##__return_queue[i];                                                                                \
        _MOCK_CALLBACK_3(_func)                                                                                        \
        _func##__call_count++;                                                                                         \
        return ret;                                                                                                    \
    }                                                                                                                  \
    _MOCK_RESET_R_SIMPLE(_func)

/* R_4_S: returns value, 4 params, struct-safe */
#define DECLARE_MOCK_R_4_S(_func, _rtype, _t0, _t1, _t2, _t3)                                                          \
    typedef struct                                                                                                     \
    {                                                                                                                  \
        _t0 p0;                                                                                                        \
        _t1 p1;                                                                                                        \
        _t2 p2;                                                                                                        \
        _t3 p3;                                                                                                        \
    } _func##_params;                                                                                                  \
    typedef void (*_func##__callback_t)(size_t, _t0, _t1, _t2, _t3);                                                   \
    extern _func##__callback_t _func##__callback;                                                                      \
    extern size_t _func##__call_count;                                                                                 \
    extern _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                               \
    extern _rtype _func##__return_queue[MOCK_CALL_STORAGE_MAX];                                                        \
    _rtype _func##__mock(_t0, _t1, _t2, _t3);                                                                          \
    void _func##__mock_reset(void)

#define DEFINE_MOCK_R_4_S(_func, _rtype, _t0, _t1, _t2, _t3)                                                           \
    _func##__callback_t _func##__callback = NULL;                                                                      \
    size_t _func##__call_count = 0;                                                                                    \
    _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                                      \
    _rtype _func##__return_queue[MOCK_CALL_STORAGE_MAX];                                                               \
    _rtype _func##__mock(_t0 _p0, _t1 _p1, _t2 _p2, _t3 _p3)                                                           \
    {                                                                                                                  \
        _func##_params *p;                                                                                             \
        _rtype ret;                                                                                                    \
        size_t i = _func##__call_count;                                                                                \
        _MOCK_REGISTER(_func)                                                                                          \
        _MOCK_OVERFLOW_CHECK_R(_func, _rtype)                                                                          \
        p = &_func##__param_history[i];                                                                                \
        _MOCK_STORE_4                                                                                                  \
        ret = _func##__return_queue[i];                                                                                \
        _MOCK_CALLBACK_4(_func)                                                                                        \
        _func##__call_count++;                                                                                         \
        return ret;                                                                                                    \
    }                                                                                                                  \
    _MOCK_RESET_R_SIMPLE(_func)

/* R_5_S: returns value, 5 params, struct-safe */
#define DECLARE_MOCK_R_5_S(_func, _rtype, _t0, _t1, _t2, _t3, _t4)                                                     \
    typedef struct                                                                                                     \
    {                                                                                                                  \
        _t0 p0;                                                                                                        \
        _t1 p1;                                                                                                        \
        _t2 p2;                                                                                                        \
        _t3 p3;                                                                                                        \
        _t4 p4;                                                                                                        \
    } _func##_params;                                                                                                  \
    typedef void (*_func##__callback_t)(size_t, _t0, _t1, _t2, _t3, _t4);                                              \
    extern _func##__callback_t _func##__callback;                                                                      \
    extern size_t _func##__call_count;                                                                                 \
    extern _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                               \
    extern _rtype _func##__return_queue[MOCK_CALL_STORAGE_MAX];                                                        \
    _rtype _func##__mock(_t0, _t1, _t2, _t3, _t4);                                                                     \
    void _func##__mock_reset(void)

#define DEFINE_MOCK_R_5_S(_func, _rtype, _t0, _t1, _t2, _t3, _t4)                                                      \
    _func##__callback_t _func##__callback = NULL;                                                                      \
    size_t _func##__call_count = 0;                                                                                    \
    _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                                      \
    _rtype _func##__return_queue[MOCK_CALL_STORAGE_MAX];                                                               \
    _rtype _func##__mock(_t0 _p0, _t1 _p1, _t2 _p2, _t3 _p3, _t4 _p4)                                                  \
    {                                                                                                                  \
        _func##_params *p;                                                                                             \
        _rtype ret;                                                                                                    \
        size_t i = _func##__call_count;                                                                                \
        _MOCK_REGISTER(_func)                                                                                          \
        _MOCK_OVERFLOW_CHECK_R(_func, _rtype)                                                                          \
        p = &_func##__param_history[i];                                                                                \
        _MOCK_STORE_5                                                                                                  \
        ret = _func##__return_queue[i];                                                                                \
        _MOCK_CALLBACK_5(_func)                                                                                        \
        _func##__call_count++;                                                                                         \
        return ret;                                                                                                    \
    }                                                                                                                  \
    _MOCK_RESET_R_SIMPLE(_func)

/* R_6_S: returns value, 6 params, struct-safe */
#define DECLARE_MOCK_R_6_S(_func, _rtype, _t0, _t1, _t2, _t3, _t4, _t5)                                                \
    typedef struct                                                                                                     \
    {                                                                                                                  \
        _t0 p0;                                                                                                        \
        _t1 p1;                                                                                                        \
        _t2 p2;                                                                                                        \
        _t3 p3;                                                                                                        \
        _t4 p4;                                                                                                        \
        _t5 p5;                                                                                                        \
    } _func##_params;                                                                                                  \
    typedef void (*_func##__callback_t)(size_t, _t0, _t1, _t2, _t3, _t4, _t5);                                         \
    extern _func##__callback_t _func##__callback;                                                                      \
    extern size_t _func##__call_count;                                                                                 \
    extern _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                               \
    extern _rtype _func##__return_queue[MOCK_CALL_STORAGE_MAX];                                                        \
    _rtype _func##__mock(_t0, _t1, _t2, _t3, _t4, _t5);                                                                \
    void _func##__mock_reset(void)

#define DEFINE_MOCK_R_6_S(_func, _rtype, _t0, _t1, _t2, _t3, _t4, _t5)                                                 \
    _func##__callback_t _func##__callback = NULL;                                                                      \
    size_t _func##__call_count = 0;                                                                                    \
    _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];                                                      \
    _rtype _func##__return_queue[MOCK_CALL_STORAGE_MAX];                                                               \
    _rtype _func##__mock(_t0 _p0, _t1 _p1, _t2 _p2, _t3 _p3, _t4 _p4, _t5 _p5)                                         \
    {                                                                                                                  \
        _func##_params *p;                                                                                             \
        _rtype ret;                                                                                                    \
        size_t i = _func##__call_count;                                                                                \
        _MOCK_REGISTER(_func)                                                                                          \
        _MOCK_OVERFLOW_CHECK_R(_func, _rtype)                                                                          \
        p = &_func##__param_history[i];                                                                                \
        _MOCK_STORE_6                                                                                                  \
        ret = _func##__return_queue[i];                                                                                \
        _MOCK_CALLBACK_6(_func)                                                                                        \
        _func##__call_count++;                                                                                         \
        return ret;                                                                                                    \
    }                                                                                                                  \
    _MOCK_RESET_R_SIMPLE(_func)

#endif /* LFG_CTEST_MOCK_H_ */
