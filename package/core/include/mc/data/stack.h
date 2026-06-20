#ifndef MC_DATA_STACK_H
#define MC_DATA_STACK_H

#include <mc/error.h>
#include <mc/data/alloc.h>

#include <stddef.h>

typedef struct MC_Stack MC_Stack;
typedef unsigned MC_StackChunkSize;
typedef size_t MC_StackCursor;

MC_Error mc_stack_init(MC_Stack **stack, MC_Alloc *base);
void mc_stack_destroy(MC_Stack *stack);

MC_Error mc_stack_reserve(MC_Stack *stack, MC_StackChunkSize size);
size_t mc_stack_top(const MC_Stack *stack);
MC_Error mc_stack_settop(MC_Stack *stack, size_t top);

MC_Alloc *mc_stack_allocator(MC_Stack *stack);

MC_Error mc_stack_alloc(MC_Stack *stack, MC_StackChunkSize size, void **mem);

#endif // MC_DATA_STACK_H
