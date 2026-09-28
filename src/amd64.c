#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ir.h"

// System V AMD64 integer argument registers.
static const char *ARG_REGS[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
#define N_ARG_REGS 6

static void ld(FILE *f, const char *reg, size_t t) {
  fprintf(f, "  mov %s, [rbp - %zu]\n", reg, t * 8);
}

static void st(FILE *f, size_t t, const char *reg) {
  fprintf(f, "  mov [rbp - %zu], %s\n", t * 8, reg);
}

static const char *setcc_for(irop_t op) {
  switch (op) {
  case OP_EQ: return "sete";
  case OP_NE: return "setne";
  case OP_GT: return "setg";
  case OP_GE: return "setge";
  case OP_LT: return "setl";
  case OP_LE: return "setle";
  default: return NULL;
  }
}

static void asm_op(FILE *f, const IrNode *n, size_t *alloc_i, size_t max_t) {
  size_t d = n->temp_dest, a = n->temp_1, b = n->temp_2;

  switch (n->op) {
  case OP_CONST:
    fprintf(f, "  mov rax, %zu\n", n->imm);
    st(f, d, "rax");
    break;

  case OP_DATA:
    fprintf(f, "  lea rax, [rel __ro_data_%zu]\n", n->imm);
    st(f, d, "rax");
    break;

  case OP_ALLOC:
    fprintf(f, "  lea rax, [rbp - %zu]\n", (max_t + (*alloc_i)++) * 8);
    st(f, d, "rax");
    break;

  case OP_ADDRESS:
    ld(f, "rax", a);
    st(f, d, "rax");
    break;

  case OP_LOAD:
    ld(f, "rax", a);
    fprintf(f, "  mov rax, [rax]\n");
    st(f, d, "rax");
    break;

  case OP_STORE:
    ld(f, "rax", b);
    ld(f, "rcx", a);
    fprintf(f, "  mov [rcx], rax\n");
    break;

  case OP_GLOBAL_L:
    fprintf(f, "  mov rax, [rel var_%s]\n", n->name);
    st(f, d, "rax");
    break;

  case OP_GLOBAL_S:
    ld(f, "rax", b);
    fprintf(f, "  mov [rel var_%s], rax\n", n->name);
    break;

  case OP_GLOBAL_ADDR:
    fprintf(f, "  lea rax, [rel var_%s]\n", n->name);
    st(f, d, "rax");
    break;

  case OP_NEG:
    ld(f, "rax", a);
    fprintf(f, "  neg rax\n");
    st(f, d, "rax");
    break;

  case OP_ADD:
  case OP_SUB:
  case OP_AND:
  case OP_OR: {
    const char *mn = n->op == OP_ADD   ? "add"
                     : n->op == OP_SUB ? "sub"
                     : n->op == OP_AND ? "and"
                                       : "or";
    ld(f, "rax", a);
    ld(f, "rcx", b);
    fprintf(f, "  %s rax, rcx\n", mn);
    st(f, d, "rax");
    break;
  }

  case OP_MUL:
    ld(f, "rax", a);
    ld(f, "rcx", b);
    fprintf(f, "  imul rax, rcx\n");
    st(f, d, "rax");
    break;

  case OP_DIV:
  case OP_MOD:
    ld(f, "rax", a);
    ld(f, "rcx", b);
    fprintf(f, "  cqo\n  idiv rcx\n");
    st(f, d, n->op == OP_DIV ? "rax" : "rdx");
    break;

  case OP_SHL:
  case OP_SHR:
    ld(f, "rax", a);
    ld(f, "rcx", b);
    fprintf(f, "  %s rax, cl\n", n->op == OP_SHL ? "shl" : "sar");
    st(f, d, "rax");
    break;

  case OP_EQ:
  case OP_NE:
  case OP_GT:
  case OP_GE:
  case OP_LT:
  case OP_LE:
    ld(f, "rax", a);
    ld(f, "rcx", b);
    fprintf(f, "  cmp rax, rcx\n  %s al\n  movzx rax, al\n", setcc_for(n->op));
    st(f, d, "rax");
    break;

  default:
    fprintf(stderr, "FATAL: asm backend: unhandled op %d\n", (int)n->op);
    exit(EXIT_FAILURE);
  }
}

static void asm_call(FILE *f, const IrNode *n) {
  size_t argc = n->params;
  size_t stack_args = argc > N_ARG_REGS ? argc - N_ARG_REGS : 0;
  size_t pad = (stack_args % 2 != 0) ? 1 : 0;

  if (pad) {
    fprintf(f, "  sub rsp, 8\n");
  }

  for (size_t i = argc; i > N_ARG_REGS; --i) {
    ld(f, "rax", n->args[i - 1]);
    fprintf(f, "  push rax\n");
  }
  for (size_t i = 0; i < argc && i < N_ARG_REGS; ++i) {
    ld(f, ARG_REGS[i], n->args[i]);
  }

  fprintf(f, "  xor eax, eax\n");
  fprintf(f, "  call %s\n", n->name);

  if (stack_args + pad) {
    fprintf(f, "  add rsp, %zu\n", (stack_args + pad) * 8);
  }
  st(f, n->temp_dest, "rax");
}

static void asm_function(FILE *f, const IrNode *fn) {
  putc('\n', f);
  size_t max_t = fn->params, n_alloc = 0;
  for (const IrNode *t = fn->nodes; t; t = t->next) {
    if (t->kind != IR_OPERATION && t->kind != IR_CALL) {
      continue;
    }
    if (t->temp_dest > max_t) {
      max_t = t->temp_dest;
    }
    if (t->kind == IR_OPERATION && t->op == OP_ALLOC) {
      n_alloc++;
    }
  }

  size_t total_slots = max_t + n_alloc;
  size_t frame = total_slots * 8;
  frame = (frame + 15) & ~(size_t)15; // To keep rsp 16-byte aligned

  fprintf(f, "global %s\n%s:\n", fn->name, fn->name);
  fprintf(f, "  push rbp\n  mov rbp, rsp\n");
  if (frame) {
    fprintf(f, "  sub rsp, %zu\n", frame);
  }

  for (size_t i = 0; i < fn->params; ++i) {
    if (i < N_ARG_REGS) {
      st(f, i + 1, ARG_REGS[i]);
    } else {
      fprintf(f, "  mov rax, [rbp + %zu]\n", 16 + (i - N_ARG_REGS) * 8);
      st(f, i + 1, "rax");
    }
  }

  size_t alloc_i = 1;
  for (const IrNode *t = fn->nodes; t; t = t->next) {
    switch (t->kind) {
    case IR_LABEL: fprintf(f, ".L%zu:\n", t->lable_id); break;
    case IR_OPERATION: asm_op(f, t, &alloc_i, max_t); break;
    case IR_EXTRN: fprintf(f, "  extern %s\n", t->name); break;
    case IR_JUMP: fprintf(f, "  jmp .L%zu\n", t->lable_id); break;
    case IR_BRANCH:
      ld(f, "rax", t->temp_dest);
      fprintf(f, "  test rax, rax\n");
      fprintf(f, "  jnz .L%zu\n", t->lable_id);
      fprintf(f, "  jmp .L%zu\n", t->lable_id_f);
      break;
    case IR_CALL: asm_call(f, t); break;
    case IR_RETURN:
      if (t->temp_dest) {
        ld(f, "rax", t->temp_dest);
      } else {
        fprintf(f, "  xor eax, eax\n");
      }
      fprintf(f, "  leave\n  ret\n");
      break;
    default: break;
    }
  }
  fputc('\n', f);
}

void dump_x86_64_nasm(Ir *ir, FILE *f) {
  fprintf(f, "; generated from %s\n", ir->module);
  fprintf(f, "default rel\n\n");

  bool have_globals = false;
  for (const IrNode *curr = ir->ir_head; curr; curr = curr->next) {
    if (curr->kind == IR_GLOBAL) {
      if (!have_globals) {
        fprintf(f, "section .bss\n");
        have_globals = true;
      }
      size_t count = curr->temp_dest ? curr->temp_dest : 1;
      fprintf(f, "global var_%s\nvar_%s: resq %zu\n", curr->name, curr->name, count);
    }
  }
  if (have_globals) {
    fputc('\n', f);
  }

  fprintf(f, "section .text\n\n");
  for (const IrNode *curr = ir->ir_head; curr; curr = curr->next) {
    if (curr->kind == IR_FUNCTION) {
      asm_function(f, curr);
    } else if (curr->kind == IR_EXTRN) {
      fprintf(f, "extern %s\n", curr->name);
    }
  }

  fprintf(f, "section .rodata\n\n");

  for (Token *tok = ir->p->l->tok_head->next; tok != NULL; tok = tok->next) {
    if (tok->kind == STRING) {
      fprintf(f, "__ro_data_%zu: db ", tok->int_lit);
      size_t len = strlen(tok->lexeme);
      for (size_t i = 0; i <= len; ++i) {
        fprintf(f, "0x%02x", (unsigned char)tok->lexeme[i]);
        if (i < len) {
          fprintf(f, ", ");
        }
      }
      fprintf(f, "\n\n");
    }
  }

  fprintf(f, "section .note.GNU-stack noalloc noexec nowrite progbits\n");
}
