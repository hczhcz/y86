#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include "y86sim.h"

void y86_push_x(Y_data *y, Y_char value) {
    if (y->x_end < &(y->x_inst[Y_X_INST_SIZE])) {
        *(y->x_end) = value;
        y->x_end++;
    } else {
        // Error
    }
}

void y86_push_x_addr(Y_data *y, Y_addr value) {
    if (y->x_end + sizeof(Y_addr) <= &(y->x_inst[Y_X_INST_SIZE])) {
        *((Y_addr *) y->x_end) = value;
        y->x_end += sizeof(Y_addr);
    } else {
        // Error
    }
}

void y86_gen_save_esp(Y_data *y) {
    y86_push_x(y, 0x89);
    y86_push_x(y, 0x25);
    y86_push_x_addr(y, &(y->ret));
}

void y86_gen_load_esp(Y_data *y) {
    y86_push_x(y, 0x8B);
    y86_push_x(y, 0x25);
    y86_push_x_addr(y, &(y->ret));
}

void y86_gen_init(Y_data *y) {
    y86_gen_save_esp(y);
}

void y86_gen_reg_update(Y_data *y, Y_reg r) {}

void y86_gen_step(Y_data *y) {}

void y86_gen_zf(Y_data *y) {}

void y86_gen_sf(Y_data *y) {}

void y86_gen_of(Y_data *y) {}

void y86_gen_pos(Y_data *y) {}

void y86_gen_stat(Y_data *y) {}

void y86_gen_ret(Y_data *y) {
    y86_gen_load_esp(y);
    y86_push_x(y, 0xC3);
}

void y86_gen_x(Y_data *y, Y_inst op, Y_reg ra, Y_reg rb, Y_word val, Y_word pos) {
    switch (op) {
        case yi_halt:
            y86_gen_ret(y);
            break;
        case yi_nop:
            //
            break;
        case yi_rrmovl:
            //
            break;
        case yi_cmovle:
            //
            break;
        case yi_cmovl:
            //
            break;
        case yi_cmove:
            //
            break;
        case yi_cmovne:
            //
            break;
        case yi_cmovge:
            //
            break;
        case yi_cmovg:
            //
            break;
        case yi_irmovl:
            //
            break;
        case yi_rmmovl:
            //
            break;
        case yi_mrmovl:
            //
            break;
        case yi_addl:
            //
            break;
        case yi_subl:
            //
            break;
        case yi_andl:
            //
            break;
        case yi_xorl:
            //
            break;        
        case yi_jmp:
            //
            break;
        case yi_jle:
            //
            break;
        case yi_jl:
            //
            break;
        case yi_je:
            //
            break;
        case yi_jne:
            //
            break;
        case yi_jge:
            //
            break;
        case yi_jg:
            //
            break;
        case yi_call:
            //
            break;
        case yi_ret:
            //
            break;
        case yi_pushl:
            //
            break;
        case yi_popl:
            //
            break;
        case yi_bad:
            //
            break;
    }
}

void y86_parse(Y_data *y, Y_char *begin, Y_char *inst, Y_char *end) {
    while (inst != end) {
        Y_inst op = *inst;
        inst++;

        Y_reg ra = yr_nil;
        Y_reg rb = yr_nil;
        Y_word val = 0;

        switch (op) {
            case yi_halt:
            case yi_nop:
            case yi_ret:
                break;
            case yi_rrmovl:
            case yi_cmovle:
            case yi_cmovl:
            case yi_cmove:
            case yi_cmovne:
            case yi_cmovge:
            case yi_cmovg:
            case yi_addl:
            case yi_subl:
            case yi_andl:
            case yi_xorl:
            case yi_pushl:
            case yi_popl:
                // Read registers
                if (inst == end) op = yi_bad;
                ra = HIGH(*inst);
                rb = LOW(*inst);
                inst++;

                break;
            case yi_irmovl:
            case yi_rmmovl:
            case yi_mrmovl:
                // Read registers
                if (inst == end) op = yi_bad;
                ra = HIGH(*inst);
                rb = LOW(*inst);
                inst++;

                // Read value
                if (inst + sizeof(Y_word) > end) op = yi_bad;
                val = *((Y_word *) inst);
                inst += sizeof(Y_word);

                break;
            case yi_jmp:
            case yi_jle:
            case yi_jl:
            case yi_je:
            case yi_jne:
            case yi_jge:
            case yi_jg:
            case yi_call:
                // Read value
                if (inst + sizeof(Y_word) > end) op = yi_bad;
                val = *((Y_word *) inst);
                inst += sizeof(Y_word);

                break;
            case yi_bad:
            default: ;
                op = yi_bad;

                break;
        }

        y86_gen_x(y, op, ra, rb, val, inst - begin);
    }
}

Y_data *y86_new() {
    Y_data *y = mmap(
        0, sizeof(Y_data),
        PROT_READ | PROT_WRITE | PROT_EXEC,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1, 0
    );

    y->x_end = &(y->x_inst[0]);
    y86_gen_init(y);

    return y;
}

void y86_exec(Y_data *y) {
    ((Y_func) y->x_inst)();
}

void y86_free(Y_data *y) {
    munmap(y, sizeof(Y_data));
}

int main() {
    Y_data *y = y86_new();
    y86_gen_x(y, yi_halt, yr_nil, yr_nil, 0, 0);
    y86_exec(y);
    printf("hello\n");
    y86_free(y);

    return 0;
}
