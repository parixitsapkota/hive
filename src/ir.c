#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "ir.h"
#include "lexer.h"

_Noreturn void ir_fatal(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  fputs("FATAL: ", stderr);
  vfprintf(stderr, fmt, ap);
  fputc('\n', stderr);
  va_end(ap);
  exit(EXIT_FAILURE);
}

Ir *init_ir(Parser *p, const char *file_path) {
  Ir *ir = calloc(1, sizeof(Ir));
  if (!ir) {
    fprintf(stderr, "FATAL: Failed to allocate ir context.\n");
    exit(EXIT_FAILURE);
  }

  ir->p = p;
  ir->module = p->l->file;
  ir->ir_arena = init_arena(1024 * sizeof(IrNode));
  ir->args_arena = init_arena(1024 * sizeof(size_t));

  ir->file = fopen(file_path, "w");
  if (!ir->file) {
    fprintf(stderr, "FATAL: Failed to open file: %s\n", file_path);
    free_arena(ir->ir_arena);
    free(ir);
    exit(EXIT_FAILURE);
  }

  return ir;
}

bool is_mem_op(irop_t op) {
  switch (op) {
  case OP_ALLOC:
  case OP_LOAD:
  case OP_STORE:
  case OP_GLOBAL_L:
  case OP_GLOBAL_S:
  case OP_GLOBAL_ADDR: return true;

  default: return false;
  }
}

const char *irop_to_str(irop_t op) {
  switch (op) {
  case OP_NEG: return "neg";
  case OP_ADD: return "add";
  case OP_SUB: return "sub";
  case OP_DIV: return "div";
  case OP_MUL: return "mul";
  case OP_MOD: return "mod";
  case OP_AND: return "and";
  case OP_OR: return "or";
  case OP_SHL: return "shl";
  case OP_SHR: return "shr";

  case OP_EQ: return "eq";
  case OP_NE: return "ne";
  case OP_GE: return "ge";
  case OP_GT: return "gt";
  case OP_LE: return "le";
  case OP_LT: return "lt";

  case OP_ALLOC: return "alloc";
  case OP_ADDRESS: return "address";
  case OP_LOAD: return "load";
  case OP_STORE: return "store";

  case OP_CONST: return "const";
  case OP_DATA: return "data";
  case OP_GLOBAL_L: return "load";
  case OP_GLOBAL_S: return "store";
  case OP_GLOBAL_ADDR: return "gaddr";

  default: return "?OP?";
  }
}

IrNode *new_ir_op(Arena *arena, size_t dest, irop_t op, size_t temp_1, size_t temp_2) {
  IrNode *node = arena_alloc(arena, sizeof(IrNode));
  *node = (IrNode){
      .kind = IR_OPERATION, .temp_dest = dest, .op = op, .temp_1 = temp_1, .temp_2 = temp_2};
  return node;
}

IrNode *new_ir_named(Arena *arena, IrKind kind, const char *name, size_t dest) {
  IrNode *node = arena_alloc(arena, sizeof(IrNode));
  *node = (IrNode){.kind = kind, .name = name, .temp_dest = dest};
  return node;
}

IrNode *new_ir_labled(Arena *arena, IrKind kind, size_t id, size_t id_f, size_t dest) {
  IrNode *node = arena_alloc(arena, sizeof(IrNode));
  *node = (IrNode){.kind = kind, .lable_id = id, .lable_id_f = id_f, .temp_dest = dest};
  return node;
}

void add_ir_node(IrNode **t_node, IrNode *node) {
  if (!node) {
    return;
  }
  node->next = NULL;
  if (*t_node) {
    (*t_node)->next = node;
  }
  *t_node = node;
}

size_t emmit_call(Ir *ir, IrNode **tail, const char *name, size_t *args, size_t params) {
  size_t t = ++ir->temp_c;
  IrNode *node = arena_alloc(ir->ir_arena, sizeof(IrNode));
  *node = (IrNode){.kind = IR_CALL, .temp_dest = t, .name = name, .args = args, .params = params};
  add_ir_node(tail, node);
  return t;
}

size_t emit_const(Ir *ir, irop_t kind, IrNode **tail, size_t value) {
  size_t t = ++ir->temp_c;
  IrNode *n = new_ir_op(ir->ir_arena, t, kind, 0, 0);
  n->imm = value;
  add_ir_node(tail, n);
  return t;
}

size_t load_global(Ir *ir, IrNode **tail, const char *name) {
  size_t t = ++ir->temp_c;
  IrNode *n = new_ir_op(ir->ir_arena, t, OP_GLOBAL_L, 0, 0);
  n->name = name;
  add_ir_node(tail, n);
  return t;
}

void store_global(Ir *ir, IrNode **tail, const char *name, size_t value) {
  IrNode *n = new_ir_op(ir->ir_arena, 0, OP_GLOBAL_S, 0, value);
  n->name = name;
  add_ir_node(tail, n);
}

size_t emit_global_addr(Ir *ir, IrNode **tail, const char *name) {
  size_t t = ++ir->temp_c;
  IrNode *n = new_ir_op(ir->ir_arena, t, OP_GLOBAL_ADDR, 0, 0);
  n->name = name;
  add_ir_node(tail, n);
  return t;
}

size_t emit_op(Ir *ir, IrNode **tail, irop_t op, size_t a, size_t b) {
  size_t t = ++ir->temp_c;
  add_ir_node(tail, new_ir_op(ir->ir_arena, t, op, a, b));
  return t;
}

void emit_store(Ir *ir, IrNode **tail, size_t value, size_t addr) {
  add_ir_node(tail, new_ir_op(ir->ir_arena, 0, OP_STORE, addr, value));
}

VarInfo *lookup_symbol(AstScope *scope, const char *name) {
  AstScope *curr_scope = scope;
  while (curr_scope != NULL) {
    if (curr_scope->symtab && has_in_hash_set(curr_scope->symtab, name)) {
      return (VarInfo *)get_from_hash_set(curr_scope->symtab, name);
    }
    curr_scope = curr_scope->parent;
  }
  return NULL;
}

VarInfo *lookup_or_die(AstScope *scope, const char *name) {
  VarInfo *var = lookup_symbol(scope, name);
  if (!var) {
    ir_fatal("Undefined variable '%s'", name);
  }
  return var;
}

irop_t binop_to_irop(int tok) {
  switch (tok) {
  case ADD: return OP_ADD;
  case SUB: return OP_SUB;
  case MUL: return OP_MUL;
  case DEV: return OP_DIV;
  case MOD: return OP_MOD;
  case BIT_AND: return OP_AND;
  case BIT_OR: return OP_OR;
  case BITSHIFT_L: return OP_SHL;
  case BITSHIFT_R: return OP_SHR;
  case EQUAL: return OP_EQ;
  case NOT_EQUAL: return OP_NE;
  case GREATER: return OP_GT;
  case GREATER_EQUAL: return OP_GE;
  case LESSER: return OP_LT;
  case LESSER_EQUAL: return OP_LE;
  default: return 0;
  }
}

typedef enum { LV_LOCAL, LV_GLOBAL } LvKind;

typedef struct {
  LvKind kind;
  size_t addr_temp;
  const char *name;
} LValue;

size_t ir_expr_f(Ir *ir, AstNode *node, AstScope *scope, IrNode **block_tail);

LValue ir_lvalue(Ir *ir, AstNode *node, AstScope *scope, IrNode **block_tail) {
  if (!node) {
    ir_fatal("Missing lvalue");
  }

  if (node->kind == AST_ATOM && node->atom_n->kind == IDENTIFIER) {
    const char *name = node->atom_n->value;
    VarInfo *var = lookup_or_die(scope, name);
    if (var->kind == AUTO_VAR || var->kind == PARAM_VAR) {
      if (var->temp_dest == 0) {
        ir_fatal("Variable '%s' has no storage slot yet", name);
      }
      return (LValue){.kind = LV_LOCAL, .addr_temp = var->temp_dest};
    } else if (var->kind == GLOBAL_VAR) {
      return (LValue){.kind = LV_GLOBAL, .name = name};
    }
    ir_fatal("Unsupported variable kind for '%s'", name);
  }

  if (node->kind == AST_UNARY && node->unary_n->op == MUL) {
    size_t addr = ir_expr_f(ir, node->unary_n->node, scope, block_tail);
    return (LValue){.kind = LV_LOCAL, .addr_temp = addr};
  }

  ir_fatal("Expression is not an lvalue");
}

size_t ir_load_lvalue(Ir *ir, IrNode **block_tail, LValue lv) {
  if (lv.kind == LV_GLOBAL) {
    return load_global(ir, block_tail, lv.name);
  }
  return emit_op(ir, block_tail, OP_LOAD, lv.addr_temp, 0);
}

void ir_store_lvalue(Ir *ir, IrNode **block_tail, size_t value, LValue lv) {
  if (lv.kind == LV_GLOBAL) {
    store_global(ir, block_tail, lv.name, value);
  } else {
    emit_store(ir, block_tail, value, lv.addr_temp);
  }
}

size_t ir_addr_of_lvalue(Ir *ir, IrNode **block_tail, LValue lv) {
  if (lv.kind == LV_GLOBAL) {
    return emit_global_addr(ir, block_tail, lv.name);
  }
  return emit_op(ir, block_tail, OP_ADDRESS, lv.addr_temp, 0);
}

size_t ir_expr_f(Ir *ir, AstNode *node, AstScope *scope, IrNode **block_tail) {
  if (!node) {
    return 0;
  }

  switch (node->kind) {
  case AST_ATOM: {
    switch (node->atom_n->kind) {
    case INT: return emit_const(ir, OP_CONST, block_tail, node->atom_n->int_lit);

    case IDENTIFIER: {
      const char *name = node->atom_n->value;
      VarInfo *var = lookup_or_die(scope, name);
      if (var->kind == AUTO_VAR || var->kind == PARAM_VAR) {
        if (var->temp_dest == 0) {
          ir_fatal("Variable '%s' has no storage slot yet", name);
        }
        return emit_op(ir, block_tail, OP_LOAD, var->temp_dest, 0);
      } else if (var->kind == GLOBAL_VAR) {
        return load_global(ir, block_tail, name);
      }
      ir_fatal("Undefined variable '%s'", name);
    }

    case STRING: {
      return emit_const(ir, OP_DATA, block_tail, node->atom_n->int_lit);
    }

    default:
      ir_fatal("Unhandled atom kind (%d, value=%s) in ir_expr_f", (int)node->atom_n->kind,
               node->atom_n->value);
    }
  }

  case AST_UNARY: {
    int op = node->unary_n->op;
    AstNode *operand = node->unary_n->node;

    switch (op) {
    case BIT_AND: {
      LValue lv = ir_lvalue(ir, operand, scope, block_tail);
      return ir_addr_of_lvalue(ir, block_tail, lv);
    }

    case MUL: {
      size_t addr = ir_expr_f(ir, operand, scope, block_tail);
      return emit_op(ir, block_tail, OP_LOAD, addr, 0);
    }

    case SUB: {
      size_t v = ir_expr_f(ir, operand, scope, block_tail);
      return emit_op(ir, block_tail, OP_NEG, v, 0);
    }

    case NOT: {
      size_t v = ir_expr_f(ir, operand, scope, block_tail);
      size_t zero = emit_const(ir, OP_CONST, block_tail, 0);
      return emit_op(ir, block_tail, OP_EQ, v, zero);
    }

    case INC:
    case DEC: {
      LValue lv = ir_lvalue(ir, operand, scope, block_tail);
      size_t old = ir_load_lvalue(ir, block_tail, lv);
      size_t one = emit_const(ir, OP_CONST, block_tail, 1);
      size_t nv = emit_op(ir, block_tail, op == INC ? OP_ADD : OP_SUB, old, one);
      ir_store_lvalue(ir, block_tail, nv, lv);
      return nv;
    }

    default: ir_fatal("Unhandled unary op (%d) in ir_expr_f", op);
    }
  }

  case AST_BINARY: {
    int op = node->binary_n->op;

    if (op == ASSIGN) {
      size_t val = ir_expr_f(ir, node->binary_n->right, scope, block_tail);
      LValue lv = ir_lvalue(ir, node->binary_n->left, scope, block_tail);
      ir_store_lvalue(ir, block_tail, val, lv);
      return val;
    }

    irop_t irop = binop_to_irop(op);
    if (!irop) {
      ir_fatal("Unhandled binary op (%d) in ir_expr_f", op);
    }
    size_t a = ir_expr_f(ir, node->binary_n->left, scope, block_tail);
    size_t b = ir_expr_f(ir, node->binary_n->right, scope, block_tail);
    return emit_op(ir, block_tail, irop, a, b);
  }

  case AST_FUNCTION_CALL: {
    AstNode *arg = node->function_call_n->args;
    size_t argc = node->function_call_n->argc;
    const char *name = node->function_call_n->name;
    size_t *args = arena_alloc(ir->args_arena, argc * sizeof(size_t));

    for (size_t i = 0; i < argc; ++i) {
      args[i] = ir_expr_f(ir, arg, scope, block_tail);
      arg = arg->next;
    }
    return emmit_call(ir, block_tail, name, args, argc);
  }

  default: ir_fatal("Unhandled node kind (%d) in ir_expr_f", (int)node->kind);
  }
}

void ir_auto_s(Ir *ir, AstNode *curr, AstScope *scope, IrNode **block_tail) {
  VarInfo *info = lookup_symbol(scope, curr->var_n->name);
  if (!info) {
    ir_fatal("Symbol '%s' not found in scope", curr->var_n->name);
  }
  info->temp_dest = emit_op(ir, block_tail, OP_ALLOC, 0, 0);
}

void ir_lable_s(Ir *ir, AstNode *curr, IrNode **block_tail) {
  if (ir->t_lable_tab && has_in_hash_set(ir->t_lable_tab, curr->name_s)) {
    VarInfo *info = get_from_hash_set(ir->t_lable_tab, curr->name_s);
    if (info->temp_dest == 0) {
      info->temp_dest = ++(ir->lable_c);
    }
    add_ir_node(block_tail, new_ir_labled(ir->ir_arena, IR_LABEL, info->temp_dest, 0, 0));
  } else {
    ir_fatal("No lable %s in current resolution.", curr->name_s);
  }
}

void ir_goto_s(Ir *ir, AstNode *curr, IrNode **block_tail) {
  if (ir->t_lable_tab && has_in_hash_set(ir->t_lable_tab, curr->name_s)) {
    VarInfo *info = get_from_hash_set(ir->t_lable_tab, curr->name_s);
    if (info->temp_dest == 0) {
      info->temp_dest = ++(ir->lable_c);
    }
    add_ir_node(block_tail, new_ir_labled(ir->ir_arena, IR_JUMP, info->temp_dest, 0, 0));
  } else {
    ir_fatal("No lable %s in current resolution.", curr->name_s);
  }
}

void ir_extrn_s(Ir *ir, AstNode *curr) {
  add_ir_node(&ir->ir_tail, new_ir_named(ir->ir_arena, IR_EXTRN, curr->name_s, 0));
}

void ir_return_s(Ir *ir, AstNode *curr, AstScope *scope, IrNode **block_tail) {
  size_t ret_temp = ir_expr_f(ir, curr->node, scope, block_tail);
  add_ir_node(block_tail, new_ir_named(ir->ir_arena, IR_RETURN, NULL, ret_temp));
}

void ir_statements(Ir *ir, AstNode *curr, AstScope *scope, IrNode **block_tail);

void ir_scope(Ir *ir, AstNode *scope_node, IrNode **block_tail) {
  if (!scope_node || !scope_node->scope_n) {
    ir_fatal("no scope!");
  }

  AstScope *inner_scope = scope_node->scope_n;
  for (AstNode *curr = inner_scope->body; curr != NULL; curr = curr->next) {
    ir->t_node = curr;
    ir_statements(ir, curr, inner_scope, block_tail);
  }

  if (inner_scope->symtab) {
    free_hash_set(inner_scope->symtab);
  }
}

void ir_if_chain_s(Ir *ir, AstNode *curr, AstScope *scope, size_t end_label_id,
                   IrNode **block_tail) {
  if (!curr) {
    return;
  }

  if (curr->kind == AST_IF || curr->kind == AST_ELSE_IF) {
    size_t then_label = ++(ir->lable_c);
    size_t next_label = ++(ir->lable_c);

    size_t condition = ir_expr_f(ir, curr->if_n->Condition, scope, block_tail);
    add_ir_node(block_tail,
                new_ir_labled(ir->ir_arena, IR_BRANCH, then_label, next_label, condition));
    add_ir_node(block_tail, new_ir_labled(ir->ir_arena, IR_LABEL, then_label, 0, 0));

    if (curr->if_n->body && curr->if_n->body->kind == AST_SCOPE) {
      ir_scope(ir, curr->if_n->body, block_tail);
    }

    add_ir_node(block_tail, new_ir_labled(ir->ir_arena, IR_JUMP, end_label_id, 0, 0));
    add_ir_node(block_tail, new_ir_labled(ir->ir_arena, IR_LABEL, next_label, 0, 0));

    if (curr->if_n->chain) {
      ir_if_chain_s(ir, curr->if_n->chain, scope, end_label_id, block_tail);
    }
  } else if (curr->kind == AST_ELSE) {
    if (curr->kind == AST_SCOPE || curr->scope_n) {
      ir_scope(ir, curr, block_tail);
    }
  }
}

void ir_if_s(Ir *ir, AstNode *node, AstScope *scope, IrNode **block_tail) {
  size_t end_label_id = ++(ir->lable_c);
  ir_if_chain_s(ir, node, scope, end_label_id, block_tail);
  add_ir_node(block_tail, new_ir_labled(ir->ir_arena, IR_LABEL, end_label_id, 0, 0));
}

void ir_while_s(Ir *ir, AstNode *node, AstScope *scope, IrNode **block_tail) {
  size_t cond_label = ++(ir->lable_c);
  size_t body_label = ++(ir->lable_c);
  size_t exit_label = ++(ir->lable_c);

  add_ir_node(block_tail, new_ir_labled(ir->ir_arena, IR_LABEL, cond_label, 0, 0));

  size_t condition = ir_expr_f(ir, node->while_n->Condition, scope, block_tail);
  add_ir_node(block_tail,
              new_ir_labled(ir->ir_arena, IR_BRANCH, body_label, exit_label, condition));
  add_ir_node(block_tail, new_ir_labled(ir->ir_arena, IR_LABEL, body_label, 0, 0));

  if (node->while_n->body && node->while_n->body->kind == AST_SCOPE) {
    ir_scope(ir, node->while_n->body, block_tail);
  }

  add_ir_node(block_tail, new_ir_labled(ir->ir_arena, IR_JUMP, cond_label, 0, 0));
  add_ir_node(block_tail, new_ir_labled(ir->ir_arena, IR_LABEL, exit_label, 0, 0));
}

void ir_statements(Ir *ir, AstNode *curr, AstScope *scope, IrNode **block_tail) {
  switch (curr->kind) {
  case AST_RETURN: ir_return_s(ir, curr, scope, block_tail); break;
  case AST_SCOPE: ir_scope(ir, curr, block_tail); break;
  case AST_AUTO: ir_auto_s(ir, curr, scope, block_tail); break;
  case AST_EXTRN: ir_extrn_s(ir, curr); break;
  case AST_EXPR: ir_expr_f(ir, curr->node, scope, block_tail); break;
  case AST_IF: ir_if_s(ir, curr, curr->if_n->body->scope_n->parent, block_tail); break;
  case AST_WHILE: ir_while_s(ir, curr, curr->while_n->body->scope_n->parent, block_tail); break;
  case AST_LABLE: ir_lable_s(ir, curr, block_tail); break;
  case AST_GOTO: ir_goto_s(ir, curr, block_tail); break;
  default: ir_fatal("Unhandled node kind (%d) in ir_statements", (int)curr->kind);
  }
}

IrNode *ir_function_s(Ir *ir, AstNode *func) {
  const char *func_name = func->function_n->name;
  size_t params = func->function_n->params;
  AstNode *body_scope = func->function_n->body;

  ir->t_lable_tab = func->function_n->lable_tab;
  ir->temp_c = params;
  ir->lable_c = 0;

  size_t entry_label = ++(ir->lable_c);
  IrNode *entry = arena_alloc(ir->ir_arena, sizeof(IrNode));
  *entry = (IrNode){.kind = IR_LABEL, .lable_id = entry_label};
  IrNode *block_tail = entry;

  for (size_t i = 1; i <= params; ++i) {
    size_t slot = emit_op(ir, &block_tail, OP_ALLOC, 0, 0);
    emit_store(ir, &block_tail, i, slot);
    add_ir_node(&block_tail, new_ir_op(ir->ir_arena, i, OP_ADDRESS, slot, 0));
  }

  ir_scope(ir, body_scope, &block_tail);

  if (block_tail->kind != IR_RETURN) {
    size_t zero = emit_const(ir, OP_CONST, &block_tail, 0);
    add_ir_node(&block_tail, new_ir_named(ir->ir_arena, IR_RETURN, NULL, zero));
  }

  free_hash_set(func->function_n->params_tab);
  free_hash_set(func->function_n->lable_tab);

  IrNode *func_node = arena_alloc(ir->ir_arena, sizeof(IrNode));
  *func_node = (IrNode){.kind = IR_FUNCTION, .name = func_name, .params = params, .nodes = entry};
  return func_node;
}

void gen_ir(Ir *ir) {
  IrNode *module = arena_alloc(ir->ir_arena, sizeof(IrNode));
  *module = (IrNode){.kind = IR_MODULE, .name = ir->module};
  ir->ir_head = module;
  ir->ir_tail = module;

  AstNode *curr = ir->p->ast_head ? ir->p->ast_head->next : NULL;
  while (curr != NULL) {
    if (curr->kind == AST_FUNCTION) {
      ir->t_node = curr;
      add_ir_node(&ir->ir_tail, ir_function_s(ir, curr));
    } else if (curr->kind == AST_GLOBAL) {
      ir->t_node = curr;
      add_ir_node(&ir->ir_tail,
                  new_ir_named(ir->ir_arena, IR_GLOBAL, curr->var_n->name, curr->var_n->size));
    } else if (curr->kind == AST_EXTRN) {
      ir->t_node = curr;
      ir_extrn_s(ir, curr);
    }
    curr = curr->next;
  }
}

void dump_op(FILE *f, const IrNode *n) {
  const char *op = irop_to_str(n->op);
  switch (n->op) {
  case OP_CONST: fprintf(f, "  %%t%zu = &%s %zu\n", n->temp_dest, op, n->imm); break;
  case OP_DATA: fprintf(f, "  %%t%zu = &%s %%ro_%zu\n", n->temp_dest, op, n->imm); break;
  case OP_GLOBAL_L:
  case OP_GLOBAL_ADDR: fprintf(f, "  %%t%zu = &%s %s\n", n->temp_dest, op, n->name); break;
  case OP_GLOBAL_S: fprintf(f, "  &%s %s, %%t%zu\n", op, n->name, n->temp_2); break;
  case OP_ALLOC: fprintf(f, "  %%t%zu = &%s\n", n->temp_dest, op); break;
  case OP_LOAD:
  case OP_ADDRESS:
  case OP_NEG: fprintf(f, "  %%t%zu = &%s %%t%zu\n", n->temp_dest, op, n->temp_1); break;
  case OP_STORE: fprintf(f, "  &%s %%t%zu, %%t%zu\n", op, n->temp_1, n->temp_2); break;
  default:
    fprintf(f, "  %%t%zu = &%s %%t%zu, %%t%zu\n", n->temp_dest, op, n->temp_1, n->temp_2);
    break;
  }
}

void dump_ir(Ir *ir, FILE *f) {
  for (IrNode *curr = ir->ir_head; curr != NULL; curr = curr->next) {
    switch (curr->kind) {
    case IR_MODULE: fprintf(f, "def module \"%s\"\n\n", curr->name); break;
    case IR_GLOBAL: fprintf(f, "static %s %zu\n\n", curr->name, curr->temp_dest); break;
    case IR_EXTRN: fprintf(f, "extern \"%s\"\n\n", curr->name); break;

    case IR_FUNCTION: {

      fprintf(f, "func $%s(", curr->name);
      for (size_t i = 1; i <= curr->params; ++i) {
        fprintf(f, "%%t%zu", i);
        if (i != curr->params) {
          fprintf(f, ", ");
        }
      }
      fprintf(f, ") %zu {\n", curr->params);

      for (IrNode *t = curr->nodes; t != NULL; t = t->next) {
        switch (t->kind) {
        case IR_LABEL: fprintf(f, "@L%zu:\n", t->lable_id); break;
        case IR_OPERATION: dump_op(f, t); break;
        case IR_JUMP: fprintf(f, "  jmp .L%zu\n", t->lable_id); break;
        case IR_BRANCH:
          fprintf(f, "  br %%t%zu, .L%zu, .L%zu\n", t->temp_dest, t->lable_id, t->lable_id_f);
          break;
        case IR_CALL:
          fprintf(f, "  %%t%zu = call %zu, $%s(", t->temp_dest, t->params, t->name);
          for (size_t i = 0; i < t->params; ++i) {
            fprintf(f, "%%t%zu", t->args[i]);
            if (i + 1 != t->params) {
              fprintf(f, ", ");
            }
          }
          fprintf(f, ")\n");
          break;
        case IR_RETURN:
          if (t->temp_dest) {
            fprintf(f, "  ret %%t%zu\n", t->temp_dest);
          } else {
            fprintf(f, "  ret\n");
          }
          break;
        case IR_MODULE:
        case IR_EXTRN:
        case IR_GLOBAL:
        case IR_FUNCTION: break;
        }
      }
      fprintf(f, "}\n\n");
      break;
    }
    default: break;
    }
  }

  for (Token *tok = ir->p->l->tok_head->next; tok != NULL; tok = tok->next) {
    if (tok->kind == STRING) {
      fprintf(f, "data ro_%zu {", tok->int_lit);
      size_t len = strlen(tok->lexeme);
      for (size_t i = 0; i <= len; ++i) {
        fprintf(f, "0x%02x", (unsigned char)tok->lexeme[i]);
        if (i < len) {
          fprintf(f, ", ");
        }
      }
      fprintf(f, "}\n");
    }
  }
}

void free_ir(Ir *ir) {
  if (ir->file) {
    fclose(ir->file);
  }
  free_arena(ir->ir_arena);
  free_arena(ir->args_arena);
  free(ir);
}
