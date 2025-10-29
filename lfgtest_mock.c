/**
 * @file
 * @brief       LFG Mocking API.
 */

/*============================================================================
 *  Includes
 *==========================================================================*/

#include <assert.h>
#include <stdlib.h>
#include "lfgtest_mock.h"

/*============================================================================
 *  Defines/Typedefs
 *==========================================================================*/

/*============================================================================
 *  Private Function Prototypes
 *==========================================================================*/

/*============================================================================
 *  Variables
 *==========================================================================*/

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
    }
    return p;
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
    }
    return p;
}

void mock_param_destroy(mock_param_action_t action)
{
    struct _mock_param_action *p = action;
    if (!p) return;

    /* traverse the linked-list and free each pointer */
    while(p->next)
    {
        struct _mock_param_action *copy = p->next;
        free(p);
        p = copy;
    }
    free(action); /* free top-level list item */
}

/*============================================================================
 *  Private Functions
 *==========================================================================*/

