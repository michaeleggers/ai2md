#ifndef MESTACK_H
#define MESTACK_H

typedef struct MeStack *MeStack;

MeStack mes_create(uint32_t elem_size, uint32_t elem_count);
void mes_destroy(MeStack * stack);
void mes_push(MeStack stack, void * elem);
void mes_pop(MeStack stack, void * elem);
int mes_empty(MeStack stack);

#endif
