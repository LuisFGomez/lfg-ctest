/**
 * @file
 * @brief       lfg-ctest mocking API.
 */

/*============================================================================
 *  Includes
 *==========================================================================*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "lfg_ctest_mock.h"

/*============================================================================
 *  Defines/Typedefs
 *==========================================================================*/

/*============================================================================
 *  Private Function Prototypes
 *==========================================================================*/

/*============================================================================
 *  Variables
 *==========================================================================*/

static void (*_mock_reset_registry[MOCK_REGISTRY_MAX])(void);
static size_t _mock_reset_registry_count;

/*============================================================================
 *  Public API
 *==========================================================================*/

mock_param_action_t mock_param_mem_read(mock_param_action_t action, unsigned callidx,
                                        unsigned paramidx, void *buffer, size_t buf_size)
{
    struct _mock_param_action *p, *p0;
    p0 = action;
    p = calloc(sizeof(*p), 1);
    assert(p);
    p->dir = eMOCK_PARAM_ACTION_DIR_READ;
    p->call_index = callidx;
    p->parameter_index = paramidx;
    p->buffer = buffer;
    p->buf_size = buf_size;
    if (p0)
    {
        while (p0->next) {
            p0 = p0->next;
        }
        p0->next = p;
        return action; /* return head of list */
    }
    return p; /* new head */
}

mock_param_action_t mock_param_mem_write(mock_param_action_t action, unsigned callidx,
                                         unsigned paramidx, void *buffer, size_t buf_size)
{
    struct _mock_param_action *p, *p0;
    p0 = action;
    p = calloc(sizeof(*p), 1);
    assert(p);
    p->dir = eMOCK_PARAM_ACTION_DIR_WRITE;
    p->call_index = callidx;
    p->parameter_index = paramidx;
    p->buffer = buffer;
    p->buf_size = buf_size;
    if (p0)
    {
        while (p0->next) {
            p0 = p0->next;
        }
        p0->next = p;
        return action; /* return head of list */
    }
    return p; /* new head */
}

mock_param_action_t mock_param_str_read(mock_param_action_t action, unsigned callidx,
                                        unsigned paramidx, char *buffer, size_t buf_size)
{
    struct _mock_param_action *p, *p0;
    p0 = action;
    p = calloc(sizeof(*p), 1);
    assert(p);
    p->dir = eMOCK_PARAM_ACTION_DIR_READ_STR;
    p->call_index = callidx;
    p->parameter_index = paramidx;
    p->buffer = buffer;
    p->buf_size = buf_size;
    if (p0)
    {
        while (p0->next) {
            p0 = p0->next;
        }
        p0->next = p;
        return action; /* return head of list */
    }
    return p; /* new head */
}

mock_param_action_t mock_param_str_write(mock_param_action_t action, unsigned callidx,
                                         unsigned paramidx, const char *buffer,
                                         size_t buf_size)
{
    struct _mock_param_action *p, *p0;
    p0 = action;
    p = calloc(sizeof(*p), 1);
    assert(p);
    p->dir = eMOCK_PARAM_ACTION_DIR_WRITE_STR;
    p->call_index = callidx;
    p->parameter_index = paramidx;
    p->buffer = (void *)buffer;
    p->buf_size = buf_size;
    if (p0)
    {
        while (p0->next) {
            p0 = p0->next;
        }
        p0->next = p;
        return action; /* return head of list */
    }
    return p; /* new head */
}

void _mock_register_reset(void (*reset_fn)(void))
{
    size_t i;

    for (i = 0; i < _mock_reset_registry_count; i++)
    {
        if (_mock_reset_registry[i] == reset_fn)
        {
            return; /* already registered */
        }
    }

    if (_mock_reset_registry_count >= MOCK_REGISTRY_MAX)
    {
        fprintf(stderr, "MOCK REGISTRY OVERFLOW: exceeded %d distinct mocks\n",
                MOCK_REGISTRY_MAX);
        assert(0 && "mock reset registry exceeded");
    }

    _mock_reset_registry[_mock_reset_registry_count++] = reset_fn;
}

void mock_reset_all(void)
{
    size_t i;

    for (i = 0; i < _mock_reset_registry_count; i++)
    {
        _mock_reset_registry[i]();
    }

    _mock_reset_registry_count = 0;
}

void mock_param_destroy(mock_param_action_t action)
{
    struct _mock_param_action *p = action;

    /* traverse the linked-list and free each node */
    while (p)
    {
        struct _mock_param_action *next = p->next;
        free(p);
        p = next;
    }
}

/*============================================================================
 *  Private Functions
 *==========================================================================*/

