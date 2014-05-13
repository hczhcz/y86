#ifndef _Y86_SIM_
#define _Y86_SIM_

#include <setjmp.h>

#define HIGH(pack) ((pack) >> 4 & 0xF)
#define LOW(pack) ((pack) & 0xF)

#define Y_MEM_SIZE 0x2000
#define Y_MEM_MASK 0x1FFF
#define Y_X_INST_SIZE 0x1000
#define Y_Y_INST_SIZE 0x0200
#define Y_PROTECT_MEM // Protect mem[>= mem_size]
#define Y_DEBUG
// #define Y_STEP_MAX_DEFAULT 10000

typedef char Y_char;
typedef int Y_word;
typedef unsigned int Y_size;
typedef void (*Y_func)(void);
typedef void *Y_addr;

typedef enum {
    yi_halt   = 0x00,
    yi_nop    = 0x10,
    yi_rrmovl = 0x20, yi_cmovle, yi_cmovl, yi_cmove, yi_cmovne, yi_cmovge, yi_cmovg,
    yi_irmovl = 0x30,
    yi_rmmovl = 0x40,
    yi_mrmovl = 0x50,
    yi_addl   = 0x60, yi_subl, yi_andl, yi_xorl,
    yi_jmp    = 0x70, yi_jle, yi_jl, yi_je, yi_jne, yi_jge, yi_jg,
    yi_call   = 0x80,
    yi_ret    = 0x90,
    yi_pushl  = 0xA0,
    yi_popl   = 0xB0,
    yi_bad    = 0xF0  // Non-standard: Compile error
} Y_inst;

typedef enum {
    ys_aok = 0x0, // Started (running)
    ys_hlt = 0x1, // Halted
    ys_adr = 0x2, // Address error
    ys_ins = 0x3, // Instruction error
    ys_clf = 0x4, // Non-standard: Loader error
    ys_ccf = 0x5, // Non-standard: Compiler, error
    ys_adp = 0x6, // Non-standard: ADR error caused by protected pc
    ys_inp = 0x7  // Non-standard: INS error caused by protected pc
} Y_stat;

typedef enum {
    yr_edi = 0x0,
    yr_esi = 0x1,
    yr_ebp = 0x2,
    yr_esp = 0x3,
    yr_ebx = 0x4,
    yr_edx = 0x5,
    yr_ecx = 0x6,
    yr_eax = 0x7,
    yr_cc  = 0x8, // Non-standard: Flags, ZF SF OF
    yr_rey = 0x9, // Non-standard: Y return address
    yr_rex = 0xA, // Non-standard: X return address
    yr_pc  = 0xB, // Non-standard: Y inst pointer
    yr_len = 0xC, // Non-standard: MM4: Y inst size
    yr_sx  = 0xD, // Non-standard: MM5: Step max
    yr_sc  = 0xE, // Non-standard: MM6: Step counter (decrease)
    yr_st  = 0xF  // Non-standard: MM7: Stat
} Y_reg;

// MM0: X ESP
// MM1: Y ESP
// MM2: Mid ESP
// MM3: Temp

const Y_reg yr_cnt = 0x08; // Counting, not a register
const Y_reg yr_cn2 = 0x10; // Another counting
const Y_reg yr_nil = 0x0f; // Null

typedef struct {
    Y_char y_inst[Y_Y_INST_SIZE];
    Y_char mem[Y_MEM_SIZE];
    Y_word reg[yr_cn2];
    Y_char x_inst[Y_X_INST_SIZE];
    Y_char *x_end;
    Y_char *(x_map[Y_Y_INST_SIZE]);
    jmp_buf jmp;
} Y_data;

#endif
