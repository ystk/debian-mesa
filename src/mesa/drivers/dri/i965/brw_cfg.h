/*
 * Copyright © 2012 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

#pragma once
#ifndef BRW_CFG_H
#define BRW_CFG_H

#include "brw_shader.h"

struct bblock_t;

struct bblock_link {
#ifdef __cplusplus
   DECLARE_RALLOC_CXX_OPERATORS(bblock_link)

   bblock_link(bblock_t *block)
      : block(block)
   {
   }
#endif

   struct exec_node link;
   struct bblock_t *block;
};

struct backend_instruction;

struct bblock_t {
#ifdef __cplusplus
   DECLARE_RALLOC_CXX_OPERATORS(bblock_t)

   explicit bblock_t(cfg_t *cfg);

   void add_successor(void *mem_ctx, bblock_t *successor);
   bool is_predecessor_of(const bblock_t *block) const;
   bool is_successor_of(const bblock_t *block) const;
   bool can_combine_with(const bblock_t *that) const;
   void combine_with(bblock_t *that);
   void dump(backend_shader *s) const;

   backend_instruction *start();
   const backend_instruction *start() const;
   backend_instruction *end();
   const backend_instruction *end() const;

   bblock_t *next();
   const bblock_t *next() const;
   bblock_t *prev();
   const bblock_t *prev() const;

   bool starts_with_control_flow() const;
   bool ends_with_control_flow() const;

   backend_instruction *first_non_control_flow_inst();
   backend_instruction *last_non_control_flow_inst();
#endif

   struct exec_node link;
   struct cfg_t *cfg;
   struct bblock_t *idom;

   int start_ip;
   int end_ip;

   struct exec_list instructions;
   struct exec_list parents;
   struct exec_list children;
   int num;

   unsigned cycle_count;
};

static inline struct backend_instruction *
bblock_start(struct bblock_t *block)
{
   return (struct backend_instruction *)exec_list_get_head(&block->instructions);
}

static inline const struct backend_instruction *
bblock_start_const(const struct bblock_t *block)
{
   return (const struct backend_instruction *)exec_list_get_head_const(&block->instructions);
}

static inline struct backend_instruction *
bblock_end(struct bblock_t *block)
{
   return (struct backend_instruction *)exec_list_get_tail(&block->instructions);
}

static inline const struct backend_instruction *
bblock_end_const(const struct bblock_t *block)
{
   return (const struct backend_instruction *)exec_list_get_tail_const(&block->instructions);
}

static inline struct bblock_t *
bblock_next(struct bblock_t *block)
{
   if (exec_node_is_tail_sentinel(block->link.next))
      return NULL;

   return (struct bblock_t *)block->link.next;
}

static inline const struct bblock_t *
bblock_next_const(const struct bblock_t *block)
{
   if (exec_node_is_tail_sentinel(block->link.next))
      return NULL;

   return (const struct bblock_t *)block->link.next;
}

static inline struct bblock_t *
bblock_prev(struct bblock_t *block)
{
   if (exec_node_is_head_sentinel(block->link.prev))
      return NULL;

   return (struct bblock_t *)block->link.prev;
}

static inline const struct bblock_t *
bblock_prev_const(const struct bblock_t *block)
{
   if (exec_node_is_head_sentinel(block->link.prev))
      return NULL;

   return (const struct bblock_t *)block->link.prev;
}

static inline bool
bblock_starts_with_control_flow(const struct bblock_t *block)
{
   enum opcode op = bblock_start_const(block)->opcode;
   return op == BRW_OPCODE_DO || op == BRW_OPCODE_ENDIF;
}

static inline bool
bblock_ends_with_control_flow(const struct bblock_t *block)
{
   enum opcode op = bblock_end_const(block)->opcode;
   return op == BRW_OPCODE_IF ||
          op == BRW_OPCODE_ELSE ||
          op == BRW_OPCODE_WHILE ||
          op == BRW_OPCODE_BREAK ||
          op == BRW_OPCODE_CONTINUE;
}

static inline struct backend_instruction *
bblock_first_non_control_flow_inst(struct bblock_t *block)
{
   struct backend_instruction *inst = bblock_start(block);
   if (bblock_starts_with_control_flow(block))
#ifdef __cplusplus
      inst = (struct backend_instruction *)inst->next;
#else
      inst = (struct backend_instruction *)inst->link.next;
#endif
   return inst;
}

static inline struct backend_instruction *
bblock_last_non_control_flow_inst(struct bblock_t *block)
{
   struct backend_instruction *inst = bblock_end(block);
   if (bblock_ends_with_control_flow(block))
#ifdef __cplusplus
      inst = (struct backend_instruction *)inst->prev;
#else
      inst = (struct backend_instruction *)inst->link.prev;
#endif
   return inst;
}

#ifdef __cplusplus
inline backend_instruction *
bblock_t::start()
{
   return bblock_start(this);
}

inline const backend_instruction *
bblock_t::start() const
{
   return bblock_start_const(this);
}

inline backend_instruction *
bblock_t::end()
{
   return bblock_end(this);
}

inline const backend_instruction *
bblock_t::end() const
{
   return bblock_end_const(this);
}

inline bblock_t *
bblock_t::next()
{
   return bblock_next(this);
}

inline const bblock_t *
bblock_t::next() const
{
   return bblock_next_const(this);
}

inline bblock_t *
bblock_t::prev()
{
   return bblock_prev(this);
}

inline const bblock_t *
bblock_t::prev() const
{
   return bblock_prev_const(this);
}

inline bool
bblock_t::starts_with_control_flow() const
{
   return bblock_starts_with_control_flow(this);
}

inline bool
bblock_t::ends_with_control_flow() const
{
   return bblock_ends_with_control_flow(this);
}

inline backend_instruction *
bblock_t::first_non_control_flow_inst()
{
   return bblock_first_non_control_flow_inst(this);
}

inline backend_instruction *
bblock_t::last_non_control_flow_inst()
{
   return bblock_last_non_control_flow_inst(this);
}
#endif

struct cfg_t {
#ifdef __cplusplus
   DECLARE_RALLOC_CXX_OPERATORS(cfg_t)

   cfg_t(exec_list *instructions);
   ~cfg_t();

   void remove_block(bblock_t *block);

   bblock_t *new_block();
   void set_next_block(bblock_t **cur, bblock_t *block, int ip);
   void make_block_array();
   void calculate_idom();
   static bblock_t *intersect(bblock_t *b1, bblock_t *b2);

   void dump(backend_shader *s);
   void dump_cfg();
   void dump_domtree();
#endif
   void *mem_ctx;

   /** Ordered list (by ip) of basic blocks */
   struct exec_list block_list;
   struct bblock_t **blocks;
   int num_blocks;

   bool idom_dirty;

   unsigned cycle_count;
};

/* Note that this is implemented with a double for loop -- break will
 * break from the inner loop only!
 */
#define foreach_block_and_inst(__block, __type, __inst, __cfg) \
   foreach_block (__block, __cfg)                              \
      foreach_inst_in_block (__type, __inst, __block)

/* Note that this is implemented with a double for loop -- break will
 * break from the inner loop only!
 */
#define foreach_block_and_inst_safe(__block, __type, __inst, __cfg) \
   foreach_block_safe (__block, __cfg)                              \
      foreach_inst_in_block_safe (__type, __inst, __block)

#define foreach_block(__block, __cfg)                          \
   foreach_list_typed (bblock_t, __block, link, &(__cfg)->block_list)

#define foreach_block_reverse(__block, __cfg)                  \
   foreach_list_typed_reverse (bblock_t, __block, link, &(__cfg)->block_list)

#define foreach_block_safe(__block, __cfg)                     \
   foreach_list_typed_safe (bblock_t, __block, link, &(__cfg)->block_list)

#define foreach_block_reverse_safe(__block, __cfg)             \
   foreach_list_typed_reverse_safe (bblock_t, __block, link, &(__cfg)->block_list)

#define foreach_inst_in_block(__type, __inst, __block)         \
   foreach_in_list(__type, __inst, &(__block)->instructions)

#define foreach_inst_in_block_safe(__type, __inst, __block)    \
   for (__type *__inst = (__type *)__block->instructions.head_sentinel.next, \
               *__next = (__type *)__inst->next;               \
        __next != NULL;                                        \
        __inst = __next,                                       \
        __next = (__type *)__next->next)

#define foreach_inst_in_block_reverse(__type, __inst, __block) \
   foreach_in_list_reverse(__type, __inst, &(__block)->instructions)

#define foreach_inst_in_block_reverse_safe(__type, __inst, __block) \
   foreach_in_list_reverse_safe(__type, __inst, &(__block)->instructions)

#define foreach_inst_in_block_starting_from(__type, __scan_inst, __inst) \
   for (__type *__scan_inst = (__type *)__inst->next;          \
        !__scan_inst->is_tail_sentinel();                      \
        __scan_inst = (__type *)__scan_inst->next)

#define foreach_inst_in_block_reverse_starting_from(__type, __scan_inst, __inst) \
   for (__type *__scan_inst = (__type *)__inst->prev;          \
        !__scan_inst->is_head_sentinel();                      \
        __scan_inst = (__type *)__scan_inst->prev)

#endif /* BRW_CFG_H */
