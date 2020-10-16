#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

#include "mestack.h"

struct MeStack
{
    uint32_t elem_size;
    void * data;
    uint32_t data_size;
    uint32_t data_free;
};

MeStack mes_create(uint32_t elem_size)
{
    MeStack stack;
    stack = (MeStack)malloc(sizeof(*stack));
    stack->elem_size = elem_size;
    stack->data_size = 100 * elem_size;
    stack->data_free = 0;
    stack->data = malloc(stack->data_size);

    return stack;
}

void mes_destroy(MeStack * stack)
{
    free((*stack)->data);
    (*stack)->data = 0;
    free(*stack);
    *stack = NULL;
}

void mes_push(MeStack stack, void * elem)
{
    assert(stack->data_free < stack->data_size);
    memcpy( (uint8_t*)(stack->data) + stack->data_free, elem, stack->elem_size );
    stack->data_free += stack->elem_size;
}

void mes_pop(MeStack stack, void * elem)
{
    assert(stack->data_free > 0 && "stack is empty!");
    stack->data_free -= stack->elem_size;
    memcpy( elem, (uint8_t*)(stack->data) + stack->data_free, stack->elem_size );
    memset( (uint8_t*)(stack->data) + stack->data_free, 0, stack->elem_size );
}

int mes_empty(MeStack stack)
{
    return stack->data_free > 0 ? 0 : 1;
}
