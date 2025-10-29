/**
 * @file
 * @brief       LFG mocking helpers.
 */

#ifndef LFGTEST_MOCK_H_
#define LFGTEST_MOCK_H_

/*============================================================================
 *  Includes
 *==========================================================================*/

#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

/*============================================================================
 *  Defines/Typedefs
 *==========================================================================*/

typedef void* mock_param_action_t;

enum _mock_param_action_dir {
    eMOCK_PARAM_ACTION_DIR_READ,
    eMOCK_PARAM_ACTION_DIR_WRITE,
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
 * - store @p buf_size bytes at destination @p buffer.
 * - Do this only for @p call_index -th call of the mocked function, if the
 *   mocked function is called multiple times.
 * @param[in]  action   existing memory storage object to link to, or NULL to create a
 *                      new one.
 * @param[in]  callidx  the call index to which this store operation will apply,
 *                      if the mocked function is called multiple times.
 * @param[in]  paramidx The 0-based position of the desired parameter.
 * @param[out] buffer   pointer to which data will be copied.
 * @param[in]  buf_size  number of bytes to copy into @p buffer.
 * @return          Newly-created parameter memory store object handle.
 */
mock_param_action_t mock_param_mem_read(mock_param_action_t action, unsigned callidx,
                                    unsigned paramidx, void *buffer, size_t buf_size);

/** Treat parameter as memory and write bytes to it.
 * This will instruct the mocked function to:
 * - treat an incoming parameter at position @p paramidx as a pointer
 * - store @p buf_size bytes at destination @p buffer.
 * - Do this only for @p call_index -th call of the mocked function, if the
 *   mocked function is called multiple times.
 * @param[in]  action   existing parameter action object to link to, or NULL to create a
 *                  new one.
 * @param[in]  callidx   the call index to which this store operation will apply,
 *                  if the mocked function is called multiple times.
 * @param[in]  paramidx   The 0-based position of the desired parameter.
 * @param[out] buffer  pointer to which data will be copied.
 * @param[in]  buf_size  number of bytes to copy into @p buffer.
 * @return          Newly-created parameter memory store object handle.
 */
mock_param_action_t mock_param_mem_write(mock_param_action_t action, unsigned callidx,
                                     unsigned paramidx, void *buffer, size_t buf_size);

/** Frees all linked parameter operations.
 * @param[in] action    parameter action chain to destroy.
 */
void mock_param_destroy(mock_param_action_t action);

/** maximum number of function calls to store */
#define MOCK_CALL_STORAGE_MAX  32

/** Declare a mocked function taking no parameters and returning nothing.
 */
#define DECLARE_MOCK_V_V(_func)         \
    void _func##__mock(void)

#define DECLARE_MOCK_R_V(_func, _rtype) \
    _rtype _func##__mock(void)

/** Declare a mocked function with no return value and 2 parameters.
 */
#define DECLARE_MOCK_V_2(_func, _t0, _t1)                                \
    /* structure encapsulation a function call's parameters */           \
    typedef struct {                                                     \
        _t0 p0;                                                          \
        _t1 p1;                                                          \
    } _func##_params;                                                    \
    /* array containing parameters of each function call */              \
    extern size_t _func##__call_count;                                   \
    extern _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX]; \
    extern mock_param_action_t _func##__param_actions;                   \
    /* declaration of mocked function */                                 \
    void _func##__mock(_t0, _t1);                                        \
    /* reset mocked function and variables */                            \
    void _func##__mock_reset(void)

/** Define a mocked function with no return value and 2 parameters.
 * This should be instantiated in the C implementation file.
 */
#define DEFINE_MOCK_V_2(_func, _t0, _t1)                                   \
    size_t _func##__call_count = 0;                                        \
    _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];          \
    mock_param_action_t _func##__param_actions = NULL;                \
    void _func##__mock(_t0 _p0,                                            \
                        _t1 _p1                                            \
                        ) {                                                \
        struct _mock_param_action *action = _func##__param_actions;   \
        _func##_params *p;                                                 \
        size_t i = _func##__call_count;                                    \
        if (i >= MOCK_CALL_STORAGE_MAX) {                                  \
            printf("fatal: max calls exceeded MOCK_CALL_STORAGE_MAX\r\n"); \
            return;                                                        \
        }                                                                  \
        /* store the parameter history */                                  \
        p = &_func##__param_history[i];                                    \
        p->p0 = _p0;                                                       \
        p->p1 = _p1;                                                       \
        /* return the designated return value */                           \
        /* peform the read/write memory operations for parameters */       \
        while (action) {                                                   \
            void *pparam;                                                  \
            if (action->call_index != _func##__call_count) {               \
                continue;                                                  \
            }                                                              \
            switch (action->parameter_index) {                             \
            case 0: pparam=(void*)(size_t)_p0; break;                      \
            case 1: pparam=(void*)(size_t)_p1; break;                      \
            }                                                              \
            if (eMOCK_PARAM_ACTION_DIR_READ == action->dir) {              \
                memcpy(action->buffer, pparam, action->buf_size);          \
            } else if (eMOCK_PARAM_ACTION_DIR_WRITE == action->dir) {      \
                memcpy(pparam, action->buffer, action->buf_size);          \
            } else {                                                       \
                assert(false); /* fatal error */                           \
            }                                                              \
           action = action->next;                                          \
        }                                                                  \
        _func##__call_count++; /* increment the call count */              \
        return;                                                            \
    }                                                                      \
    void _func##__mock_reset(void) {                                       \
        memset(_func##__param_history, 0, sizeof(_func##__param_history)); \
        _func##__call_count = 0;                                           \
        mock_param_destroy(_func##__param_actions);                   \
        _func##__param_actions = NULL;                                \
    }

/** Declare a mocked function with no return value and 3 parameters.
 */
#define DECLARE_MOCK_V_3(_func, _t0, _t1, _t2)                           \
    /* structure encapsulation a function call's parameters */           \
    typedef struct {                                                     \
        _t0 p0;                                                          \
        _t1 p1;                                                          \
        _t2 p2;                                                          \
    } _func##_params;                                                    \
    /* array containing parameters of each function call */              \
    extern size_t _func##__call_count;                                   \
    extern _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX]; \
    extern mock_param_action_t _func##__param_actions;                   \
    /* declaration of mocked function */                                 \
    void _func##__mock(_t0, _t1, _t2);                                   \
    /* reset mocked function and variables */                            \
    void _func##__mock_reset(void)

/** Define a mocked function with no return value and 3 parameters.
 * This should be instantiated in the C implementation file.
 */
#define DEFINE_MOCK_V_3(_func, _t0, _t1, _t2)                              \
    size_t _func##__call_count = 0;                                        \
    _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];          \
    mock_param_action_t _func##__param_actions = NULL;                     \
    void _func##__mock(_t0 _p0,                                            \
                        _t1 _p1,                                           \
                        _t2 _p2                                            \
                        ) {                                                \
        struct _mock_param_action *action = _func##__param_actions;        \
        _func##_params *p;                                                 \
        size_t i = _func##__call_count;                                    \
        if (i >= MOCK_CALL_STORAGE_MAX) {                                  \
            printf("fatal: max calls exceeded MOCK_CALL_STORAGE_MAX\r\n"); \
            return;                                                        \
        }                                                                  \
        /* store the parameter history */                                  \
        p = &_func##__param_history[i];                                    \
        p->p0 = _p0;                                                       \
        p->p1 = _p1;                                                       \
        p->p2 = _p2;                                                       \
        /* return the designated return value */                           \
        /* peform the read/write memory operations for parameters */       \
        while (action) {                                                   \
            void *pparam;                                                  \
            if (action->call_index != _func##__call_count) {               \
                continue;                                                  \
            }                                                              \
            switch (action->parameter_index) {                             \
            case 0: pparam=(void*)(size_t)_p0; break;                      \
            case 1: pparam=(void*)(size_t)_p1; break;                      \
            case 2: pparam=(void*)(size_t)_p2; break;                      \
            }                                                              \
            if (eMOCK_PARAM_ACTION_DIR_READ == action->dir) {              \
                memcpy(action->buffer, pparam, action->buf_size);          \
            } else if (eMOCK_PARAM_ACTION_DIR_WRITE == action->dir) {      \
                memcpy(pparam, action->buffer, action->buf_size);          \
            } else {                                                       \
                assert(false); /* fatal error */                           \
            }                                                              \
           action = action->next;                                          \
        }                                                                  \
        _func##__call_count++; /* increment the call count */              \
        return;                                                            \
    }                                                                      \
    void _func##__mock_reset(void) {                                       \
        memset(_func##__param_history, 0, sizeof(_func##__param_history)); \
        _func##__call_count = 0;                                           \
        mock_param_destroy(_func##__param_actions);                        \
        _func##__param_actions = NULL;                                     \
    }


/** Declare a mocked function with return value and 7 parameters.
 */
#define DECLARE_MOCK_R_7(_func, _rtype, _t0, _t1, _t2, _t3, _t4, _t5, _t6) \
    /* structure encapsulation a function call's parameters */             \
    typedef struct {                                                       \
        _t0 p0;                                                            \
        _t1 p1;                                                            \
        _t2 p2;                                                            \
        _t3 p3;                                                            \
        _t4 p4;                                                            \
        _t5 p5;                                                            \
        _t6 p6;                                                            \
    } _func##_params;                                                      \
    /* array containing parameters of each function call */                \
    extern size_t _func##__call_count;                                     \
    extern _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];   \
    extern _rtype _func##__return_queue[MOCK_CALL_STORAGE_MAX];            \
    extern mock_param_action_t _func##__param_actions;                     \
    /* declaration of mocked function */                                   \
    _rtype _func##__mock(_t0, _t1, _t2, _t3, _t4, _t5, _t6);               \
    /* reset mocked function and variables */                              \
    void _func##__mock_reset(void)

#define DEFINE_MOCK_R_7(_func, _rtype, _t0, _t1, _t2, _t3, _t4, _t5, _t6)  \
    size_t _func##__call_count = 0;                                        \
    _rtype _func##__return_queue[MOCK_CALL_STORAGE_MAX];                   \
    _func##_params _func##__param_history[MOCK_CALL_STORAGE_MAX];          \
    mock_param_action_t _func##__param_actions = NULL;                     \
    _rtype _func##__mock(_t0 _p0,                                          \
                         _t1 _p1,                                          \
                         _t2 _p2,                                          \
                         _t3 _p3,                                          \
                         _t4 _p4,                                          \
                         _t5 _p5,                                          \
                         _t6 _p6) {                                        \
        struct _mock_param_action *action = _func##__param_actions;        \
        _rtype ret;                                                        \
        _func##_params *p;                                                 \
        size_t i = _func##__call_count;                                    \
        if (i >= MOCK_CALL_STORAGE_MAX) {                                  \
            printf("fatal: max calls exceeded MOCK_CALL_STORAGE_MAX\r\n"); \
            return (_rtype)0;                                              \
        }                                                                  \
        /* store the parameter history */                                  \
        p = &_func##__param_history[i];                                    \
        p->p0 = _p0;                                                       \
        p->p1 = _p1;                                                       \
        p->p2 = _p2;                                                       \
        p->p3 = _p3;                                                       \
        p->p4 = _p4;                                                       \
        p->p5 = _p5;                                                       \
        p->p6 = _p6;                                                       \
        /* return the designated return value */                           \
        ret = _func##__return_queue[i];                                    \
        /* peform the read/write memory operations for parameters */       \
        while (action) {                                                   \
            void *pparam;                                                  \
            if (action->call_index != _func##__call_count) {               \
                continue;                                                  \
            }                                                              \
            switch (action->parameter_index) {                             \
            case 0: pparam=(void*)(size_t)_p0; break;                      \
            case 1: pparam=(void*)(size_t)_p1; break;                      \
            case 2: pparam=(void*)(size_t)_p2; break;                      \
            case 3: pparam=(void*)(size_t)_p3; break;                      \
            case 4: pparam=(void*)(size_t)_p4; break;                      \
            case 5: pparam=(void*)(size_t)_p5; break;                      \
            case 6: pparam=(void*)(size_t)_p6; break;                      \
            }                                                              \
            if (eMOCK_PARAM_ACTION_DIR_READ == action->dir) {              \
                memcpy(action->buffer, pparam, action->buf_size);          \
            } else if (eMOCK_PARAM_ACTION_DIR_WRITE == action->dir) {      \
                memcpy(pparam, action->buffer, action->buf_size);          \
            } else {                                                       \
                assert(false); /* fatal error */                           \
            }                                                              \
           action = action->next;                                          \
        }                                                                  \
        _func##__call_count++; /* increment the call count */              \
        return ret;                                                        \
    }                                                                      \
    void _func##__mock_reset(void) {                                       \
        memset(_func##__return_queue, 0, sizeof(_func##__return_queue));   \
        memset(_func##__param_history, 0, sizeof(_func##__param_history)); \
        _func##__call_count = 0;                                           \
        mock_param_destroy(_func##__param_actions);                        \
        _func##__param_actions = NULL;                                     \
    }

/*============================================================================
 *  Public API
 *==========================================================================*/


#endif /* LFGTEST_MOCK_H_ */

