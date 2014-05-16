#ifndef _Y86_SIM_
#define _Y86_SIM_

#include <setjmp.h>

#define HIGH(pack) ((pack) >> 4 & 0xF)
#define LOW(pack) ((pack) & 0xF)
#define IO_WORD(data) (*(Y_word *) (data))
#define IO_ADDR(data) (*(Y_addr *) (data))

#define Y_MEM_SIZE 0x2000
#define Y_X_INST_SIZE 0x2000
#define Y_Y_INST_SIZE 0x0200
#define Y_MASK_NOT_MEM "0xFFFFE000" // "0x1FFF"
#define Y_MASK_NOT_INST "0xFFFFFE00" // "0x01FF"
#define Y_PROTECT_MEM // Protect mem[>= mem_size]
// #define Y_STEP_MAX_DEFAULT 10000

typedef char Y_char;
typedef int Y_word;
typedef void (*Y_func)(void);
typedef Y_char *Y_addr;

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
    ys_adp = 0x6, // Non-standard: ADR error caused by mem protection
    ys_inp = 0x7, // Non-standard: INS error caused by mem protection
    ys_ima = 0x8, // Non-standard: Memory access interrupt, range checking
    ys_imc = 0x9, // Non-standard: Memory changed interrupt, check if instruction changed, load if necessary
    ys_ret = 0xA  // Non-standard: Ret interrupt, check and pop esp, map to x_inst, jump (and load if necessary)
} Y_stat;

const Y_stat ys_cnt = 0x8; // Normal stat if below

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
    yr_len = 0xC, // Non-standard: Y inst size
    yr_sx  = 0xD, // Non-standard: Step max
    yr_sc  = 0xE, // Non-standard: MM6: Step counter (decrease)
    yr_st  = 0xF  // Non-standard: MM7: Stat
} Y_reg;

// MM0: X ESP
// MM1: Y ESP
// MM2: Mid ESP
// MM3: Temp
// MM4: Mem pointer, for ys_ima and ys_imc
// MM5: ???


const Y_reg yr_cnt = 0x08; // Counting, not a register
const Y_reg yr_cn2 = 0x10; // Another counting
const Y_reg yr_nil = 0x0f; // Null

typedef struct {
    Y_char bak_mem[Y_MEM_SIZE];
    Y_char mem[Y_MEM_SIZE];
    Y_word bak_reg[yr_cn2];
    Y_word reg[yr_cn2];
    Y_char x_inst[Y_X_INST_SIZE];
    Y_addr x_end;
    Y_addr x_map[Y_Y_INST_SIZE];
    jmp_buf jmp;
} Y_data;

#endif
