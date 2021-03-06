#include "y86sim.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

Y_data *y86_new() {
    return mmap(
        0, sizeof(Y_data),
        PROT_READ | PROT_WRITE | PROT_EXEC,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1, 0
    );
}

const Y_word y_static_num[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

#define YX(data) {y86_push_x(y, data);}
#define YXW(data) {y86_push_x_word(y, data);}
#define YXA(data) {y86_push_x_addr(y, data);}

void y86_push_x(Y_data *y, Y_char value) {
    if (y->x_end < &(y->x_inst[Y_X_INST_SIZE])) {
        *(y->x_end) = value;
        y->x_end++;
    } else {
        fprintf(stderr, "Too large compiled instruction size (char: 0x%x)\n", value);
        longjmp(y->jmp, ys_ccf);
    }
}

void y86_push_x_word(Y_data *y, Y_word value) {
    if (y->x_end + sizeof(Y_word) <= &(y->x_inst[Y_X_INST_SIZE])) {
        IO_WORD(y->x_end) = value;
        y->x_end += sizeof(Y_word);
    } else {
        fprintf(stderr, "Too large compiled instruction size (word: 0x%x)\n", value);
        longjmp(y->jmp, ys_ccf);
    }
}

void y86_push_x_addr(Y_data *y, Y_addr value) {
    if (y->x_end + sizeof(Y_addr) <= &(y->x_inst[Y_X_INST_SIZE])) {
        IO_ADDR(y->x_end) = value;
        y->x_end += sizeof(Y_addr);
    } else {
        fprintf(stderr, "Too large compiled instruction size (addr: 0x%x)\n", (Y_word) value);
        longjmp(y->jmp, ys_ccf);
    }
}

void y86_link_x_map(Y_data *y, Y_word pos) {
    if (pos < Y_Y_INST_SIZE) {
        y->x_map[pos] = y->x_end;
    } else {
        fprintf(stderr, "Too large y86 instruction size\n");
        longjmp(y->jmp, ys_ccf);
    }
}

void y86_gen_before(Y_data *y, Y_word protect_esp) {
    if (protect_esp) {
        YX(0x0F) YX(0x7E) YX(0xCC) // movd %mm1, %esp
    }
}

void y86_gen_after(Y_data *y, Y_word protect_esp) {
    if (protect_esp) {
        YX(0x0F) YX(0x6E) YX(0xCC) // movd %esp, %mm1
    }
}

void y86_gen_check(Y_data *y, Y_word protect_esp) {
    if (protect_esp) {
        YX(0x0F) YX(0x7E) YX(0xD4) // movd %mm2, %esp
    }
    YX(0xFF) YX(0x14) YX(0x24) // call (%esp)
}

void y86_gen_after_goto(Y_data *y, Y_addr value, Y_word protect_esp) {
    // If changed, value of jmp_skip should be updated (for jump instruction etc.)

    y86_gen_after(y, protect_esp);
    YX(0x0F) YX(0x7E) YX(0xD4) // movd %mm2, %esp
    YX(0xFF) YX(0x35) YXA(value) // push value
    YX(0xFF) YX(0x64) YX(0x24) YX(0x04) // jmp 4(%esp)
}

void y86_gen_stat(Y_data *y, Y_stat stat) {
    YX(0x0F) YX(0x6E) YX(0x3D) YXA((Y_addr) &(y_static_num[stat])) // movd stat, %mm7
}

void y86_gen_raw_jmp(Y_data *y, Y_addr value) {
    YX(0xFF) YX(0x25) YXA(value) // jmp *value
}

Y_char y86_x_regbyte_C(Y_reg_id ra, Y_reg_id rb) {
    return 0xC0 | (ra << 3) | rb;
}

Y_char y86_x_regbyte_8(Y_reg_id ra, Y_reg_id rb) {
    return 0x80 | (ra << 3) | rb;
}

void y86_gen_protect(Y_data *y) {
    y86_gen_stat(y, ys_hlt);
    y86_gen_check(y, 0);
}

void y86_gen_interrupt_ready(Y_data *y, Y_stat stat, Y_word protect_esp) {
    y86_gen_stat(y, stat);
    y86_gen_after(y, protect_esp);
}

void y86_gen_interrupt_go(Y_data *y, Y_word protect_esp) {
    YX(0x0F) YX(0x6E) YX(0xE4) // movd %esp, %mm4
    y86_gen_check(y, 1);
    y86_gen_before(y, protect_esp);
}

void y86_gen_x(Y_data *y, Y_inst op, Y_reg_id ra, Y_reg_id rb, Y_word val) {
    Y_word protect_esp = (ra == yri_esp) || (rb == yri_esp) || ((Y_char) op < 0);
    Y_word jmp_skip = protect_esp ? 16 : 13;

    y86_gen_before(y, protect_esp);

    // Always: ra, rb >= 0
    switch (op) {
        case yi_halt:
            y86_gen_stat(y, ys_hlt);
            break;
        case yi_nop:
            // Nothing
            break;
        case yi_rrmovl:
        case yi_cmovle:
        case yi_cmovl:
        case yi_cmove:
        case yi_cmovne:
        case yi_cmovge:
        case yi_cmovg:
            if (ra < yr_cnt && rb < yr_cnt) {
                switch (op) {
                    case yi_rrmovl:
                        YX(0x89) YX(y86_x_regbyte_C(ra, rb)) // movl ...
                        break;
                    case yi_cmovle:
                        YX(0x0F) YX(0x4E) YX(y86_x_regbyte_C(rb, ra)) // cmovle ...
                        break;
                    case yi_cmovl:
                        YX(0x0F) YX(0x4C) YX(y86_x_regbyte_C(rb, ra)) // cmovl ...
                        break;
                    case yi_cmove:
                        YX(0x0F) YX(0x44) YX(y86_x_regbyte_C(rb, ra)) // cmove ...
                        break;
                    case yi_cmovne:
                        YX(0x0F) YX(0x45) YX(y86_x_regbyte_C(rb, ra)) // cmovne ...
                        break;
                    case yi_cmovge:
                        YX(0x0F) YX(0x4D) YX(y86_x_regbyte_C(rb, ra)) // cmovge ...
                        break;
                    case yi_cmovg:
                        YX(0x0F) YX(0x4F) YX(y86_x_regbyte_C(rb, ra)) // cmovg ...
                        break;
                    default:
                        // Impossible
                        fprintf(stderr, "Internal bug!\n");
                        longjmp(y->jmp, ys_ccf);
                        break;
                }
            } else {
                y86_gen_stat(y, ys_ins);
            }
            break;
        case yi_irmovl:
            if (ra == yr_nil && rb < yr_cnt) {
                YX(0xB8 + rb) YXW(val) // movl ...
            } else {
                y86_gen_stat(y, ys_ins);
            }
            break;
        case yi_rmmovl:
            if (ra < yr_cnt && rb < yr_cnt) {
                y86_gen_interrupt_ready(y, ys_ima, protect_esp);
                YX(0x8D) YX(0xA0 + rb) // leal offset(%rb), %esp
                if (rb == yri_esp) YX(0x24) // Extra byte for %esp
                YXW(val)
                y86_gen_interrupt_go(y, protect_esp);

                YX(0x89) YX(y86_x_regbyte_8(ra, rb)) // movl ...
                if (rb == yri_esp) YX(0x24) // Extra byte for %esp
                YXA(&(y->mem[val]))

                y86_gen_stat(y, ys_imc);
            } else {
                y86_gen_stat(y, ys_ins);
            }
            break;
        case yi_mrmovl:
            if (ra < yr_cnt && rb < yr_cnt) {
                y86_gen_interrupt_ready(y, ys_ima, protect_esp);
                YX(0x8D) YX(0xA0 + rb) // leal offset(%rb), %esp
                if (rb == yri_esp) YX(0x24) // Extra byte for %esp
                YXW(val)
                y86_gen_interrupt_go(y, protect_esp);

                YX(0x8B) YX(y86_x_regbyte_8(ra, rb)) // movl ...
                if (rb == yri_esp) YX(0x24) // Extra byte for %esp
                YXA(&(y->mem[val]))
            } else {
                y86_gen_stat(y, ys_ins);
            }
            break;
        case yi_addl:
        case yi_subl:
        case yi_andl:
        case yi_xorl:
            if (ra < yr_cnt && rb < yr_cnt) {
                switch (op) {
                    case yi_addl:
                        YX(0x01) YX(y86_x_regbyte_C(ra, rb)) // addl ...
                        break;
                    case yi_subl:
                        YX(0x29) YX(y86_x_regbyte_C(ra, rb)) // subl ...
                        break;
                    case yi_andl:
                        YX(0x21) YX(y86_x_regbyte_C(ra, rb)) // andl ...
                        break;
                    case yi_xorl:
                        YX(0x31) YX(y86_x_regbyte_C(ra, rb)) // xorl ...
                        break;
                    default:
                        // Impossible
                        fprintf(stderr, "Internal bug!\n");
                        longjmp(y->jmp, ys_ccf);
                        break;
                }
            } else {
                y86_gen_stat(y, ys_ins);
            }
            break;
        case yi_jmp:
        case yi_jle:
        case yi_jl:
        case yi_je:
        case yi_jne:
        case yi_jge:
        case yi_jg:
            if (val >= 0 && val < Y_Y_INST_SIZE) {
                switch (op) {
                    case yi_jmp:
                        break;
                    case yi_jle:
                        YX(0x7F) YX(jmp_skip) // jg after
                        break;
                    case yi_jl:
                        YX(0x7D) YX(jmp_skip) // jge after
                        break;
                    case yi_je:
                        YX(0x75) YX(jmp_skip) // jne after
                        break;
                    case yi_jne:
                        YX(0x74) YX(jmp_skip) // je after
                        break;
                    case yi_jge:
                        YX(0x7C) YX(jmp_skip) // jl after
                        break;
                    case yi_jg:
                        YX(0x7E) YX(jmp_skip) // jle after
                        break;
                    default:
                        // Impossible
                        fprintf(stderr, "Internal bug!\n");
                        longjmp(y->jmp, ys_ccf);
                        break;
                }

                if (!y->x_map[val]) {
                    y->x_map[val] = Y_BAD_ADDR;
                }
                y86_gen_after_goto(y, (Y_addr) &(y->x_map[val]), protect_esp);
            } else {
                y86_gen_stat(y, ys_adp);
            }
            break;
        case yi_call:
            if (val >= 0 && val < Y_Y_INST_SIZE) {
                YX(0x8D) YX(0x64) YX(0x24) YX(0xFC) // leal -4(%esp), %esp

                y86_gen_interrupt_ready(y, ys_ima, protect_esp);
                y86_gen_interrupt_go(y, protect_esp);

                YX(0x8D) YX(0xA4) YX(0x24) YXW(4 + (Y_word) &(y->mem[0])) // leal offset+4(%esp), %esp
                YX(0x68) YXW(y->reg[yr_pc] + 5) // push %pc+5
                YX(0x8D) YX(0xA4) YX(0x24) YXW(0 - (Y_word) &(y->mem[0])) // leal -offset(%esp), %esp

                y86_gen_stat(y, ys_imc);

                if (!y->x_map[val]) {
                    y->x_map[val] = Y_BAD_ADDR;
                }
                y86_gen_after_goto(y, (Y_addr) &(y->x_map[val]), protect_esp);
            } else {
                y86_gen_stat(y, ys_adp);
            }
            break;
        case yi_ret:
            y86_gen_stat(y, ys_ret);

            break;
        case yi_pushl:
            if (ra < yr_cnt && rb == yr_nil) {
                YX(0x8D) YX(0x64) YX(0x24) YX(0xFC) // leal -4(%esp), %esp

                y86_gen_interrupt_ready(y, ys_ima, protect_esp);
                y86_gen_interrupt_go(y, protect_esp);

                YX(0x8D) YX(0x64) YX(0x24) YX(0x04) // leal 4(%esp), %esp // TODO: need optimization

                YX(0x89) YX(y86_x_regbyte_8(ra, yri_esp)) // movl %ra, offset-4(%esp)
                YX(0x24) // Extra byte for %esp
                YXA(&(y->mem[0]) - 4)

                YX(0x8D) YX(0x64) YX(0x24) YX(0xFC) // leal -4(%esp), %esp

                y86_gen_stat(y, ys_imc);
            } else {
                y86_gen_stat(y, ys_ins);
            }
            break;
        case yi_popl:
            if (ra < yr_cnt && rb == yr_nil) {
                y86_gen_interrupt_ready(y, ys_ima, protect_esp);
                y86_gen_interrupt_go(y, protect_esp);

                YX(0x8D) YX(0x64) YX(0x24) YX(0x04) // leal 4(%esp), %esp

                YX(0x8B) YX(y86_x_regbyte_8(ra, yri_esp)) // movl offset-4(%esp), $ra
                YX(0x24) // Extra byte for %esp
                YXA(&(y->mem[0]) - 4)
            } else {
                y86_gen_stat(y, ys_ins);
            }
            break;
        case yi_bad:
            y86_gen_stat(y, ys_ins);
            break;
        default:
            // Impossible
            fprintf(stderr, "Internal bug!\n");
            longjmp(y->jmp, ys_ccf);
            break;
    }

    y86_gen_after(y, protect_esp);
    y86_gen_check(y, protect_esp);
}

void y86_parse(Y_data *y, Y_char **inst, Y_char *end) {
    Y_inst op = **inst & 0xFF;
    (*inst)++;

    Y_reg_id ra = yr_nil;
    Y_reg_id rb = yr_nil;
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
            if (*inst == end) op = yi_bad;
            ra = HIGH(**inst);
            rb = LOW(**inst);
            (*inst)++;

            break;

        case yi_irmovl:
        case yi_rmmovl:
        case yi_mrmovl:
            // Read registers
            if (*inst == end) op = yi_bad;
            ra = HIGH(**inst);
            rb = LOW(**inst);
            (*inst)++;

            // Read value
            if (*inst + sizeof(Y_word) > end) op = yi_bad;
            val = IO_WORD(*inst);
            *inst += sizeof(Y_word);

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
            if (*inst + sizeof(Y_word) > end) op = yi_bad;
            val = IO_WORD(*inst);
            *inst += sizeof(Y_word);

            break;

        case yi_bad:
        default:
            op = yi_bad;

            break;
    }

    y86_gen_x(y, op, ra, rb, val);
}

void y86_load_reset(Y_data *y) {
    Y_word index;
    for (index = 0; index < Y_Y_INST_SIZE; ++index) {
        y->x_map[index] = 0;
    }
    y->x_end = &(y->x_inst[0]);
}

void y86_load(Y_data *y, Y_char *begin) {
    Y_char *inst = begin;
    Y_char *end = &(y->mem[y->reg[yr_len]]);
    while (*end) {
        y->reg[yr_len]++;
        end = &(y->mem[y->reg[yr_len]]);
    }

    Y_word pc = y->reg[yr_pc];

    while (inst != end) {
        while (inst != end) {
            y->reg[yr_pc] = inst - begin;

            if (y->x_map[y->reg[yr_pc]] && y->x_map[y->reg[yr_pc]] != Y_BAD_ADDR) {
                y86_gen_raw_jmp(y, y->x_map[y->reg[yr_pc]]);
            } else {
                y86_link_x_map(y, y->reg[yr_pc]);
                y86_parse(y, &inst, end);
            }
        }

        for (inst = begin; inst != end; ++inst) {
            if (y->x_map[inst - begin] == Y_BAD_ADDR) break;
        }

        y86_link_x_map(y, y->reg[yr_pc] + 1);
        y86_gen_protect(y);
    };

    y->reg[yr_pc] = pc;
}

void y86_load_all(Y_data *y) {
    y86_load_reset(y);
    y86_load(y, &(y->mem[0]));
}

void y86_load_file_bin(Y_data *y, FILE *binfile) {
    clearerr(binfile);

    y->reg[yr_len] = fread(&(y->mem[0]), sizeof(Y_char), Y_Y_INST_SIZE, binfile);
    if (ferror(binfile)) {
        fprintf(stderr, "fread() failed (0x%x)\n", y->reg[yr_len]);
        longjmp(y->jmp, ys_clf);
    }
    if (!feof(binfile)) {
        fprintf(stderr, "Too large memory footprint (0x%x)\n", y->reg[yr_len]);
        longjmp(y->jmp, ys_clf);
    }
}

void y86_load_file(Y_data *y, Y_char *fname) {
    FILE *binfile = fopen(fname, "rb");

    if (binfile) {
        y86_load_file_bin(y, binfile);
        fclose(binfile);
    } else {
        fprintf(stderr, "Can't open binary file '%s'\n", fname);
        longjmp(y->jmp, ys_clf);
    }
}

void y86_ready(Y_data *y, Y_word step) {
    memcpy(&(y->bak_mem[0]), &(y->mem[0]), sizeof(y->bak_mem));
    memcpy(&(y->bak_reg[0]), &(y->reg[0]), sizeof(y->bak_reg));

    y->reg[yr_cc] = 0x40;
    y->reg[yr_sx] = step;
    y->reg[yr_sc] = step;
    y->reg[yr_st] = ys_aok;
}

void y86_trace_ip(Y_data *y) {
    if (y->x_map[y->reg[yr_pc]]) {
        y->reg[yr_rey] = (Y_word) y->x_map[y->reg[yr_pc]];
    } else {
        y86_load(y, &(y->mem[y->reg[yr_pc]]));
    }
}

void __attribute__ ((noinline)) y86_exec(Y_data *y) {
    __asm__ __volatile__(
        "pushal" "\n\t"
        "pushf" "\n\t"

        "movd %%esp, %%mm0" "\n\t"
        "movl %0, %%esp" "\n\t"

        // Load data
        "movd 12(%%esp), %%mm1" "\n\t"
        "popal" "\n\t"
        "movd 24(%%esp), %%mm6" "\n\t"
        "movd 28(%%esp), %%mm7" "\n\t"

        // Build callback stack
        "addl $12, %%esp" "\n\t"
        "pushl $y86_check" "\n\t"
        "movd %%esp, %%mm2" "\n\t"

        "subl $8, %%esp" "\n\t"
        "popf" "\n\t"

    // Checking before calling
    "y86_check:" "\n\t"

        "pushf" "\n\t"
        "movd %%eax, %%mm3" "\n\t"

        // Check state
        "movd %%mm7, %%eax" "\n\t"
        "testl %%eax, %%eax" "\n\t"
        "jnz y86_int" "\n\t"

    "y86_check_2:" "\n\t"
        // Check step
        "movd %%mm6, %%eax" "\n\t"
        "decl %%eax" "\n\t"
        "movd %%eax, %%mm6" "\n\t"
        "js y86_fin" "\n\t"

    // Call the function
    "y86_call:" "\n\t"

        "movd %%mm3, %%eax" "\n\t"
        "popf" "\n\t"

        "ret" "\n\t"

    ".align 4" "\n\t"

    "y_st_jump:" "\n\t"
        // If stat == 8 (ys_ima), do mem adr checking
        ".long y86_int_ima" "\n\t"
        // If stat == 9 (ys_imc), do inst adr checking
        ".long y86_int_imc" "\n\t"
        // If stat == 10 (ys_ret), handle by outer
        ".long y86_fin" "\n\t"

    ".align 16, 0x90" "\n\t"

    // Handling interrupt etc.
    "y86_int:" "\n\t"

        // If stat < 8, just finished
        "subl $8, %%eax" "\n\t"
        "js y86_int_brk" "\n\t"

        "leal y_st_jump(, %%eax, 4), %%eax" "\n\t"
        "jmpl *(%%eax)" "\n\t"

        "y86_int_ima:" "\n\t"

            // If mm4 + 3 < mem_size, safe, else adr error
            "movd %%mm4, %%eax" "\n\t"

            "andl $" Y_MASK_NOT_MEM ", %%eax" "\n\t"
            "jnz y86_int_brk" "\n\t"

            "xorl %%eax, %%eax" "\n\t"
            "movd %%eax, %%mm7" "\n\t" // Assert: eax is 0
            "jmp y86_call" "\n\t"

        "y86_int_imc:" "\n\t"

            // If mm4 <= current inst size, handle by outer
            "movd %%mm4, %%eax" "\n\t"

            "cmpl 16(%%esp), %%eax" "\n\t"
            "jg y86_check_2" "\n\t"

            "jmp y86_fin" "\n\t"

        "y86_int_brk:" "\n\t"

            "movd %%mm6, %%eax" "\n\t"
            "decl %%eax" "\n\t"
            "movd %%eax, %%mm6" "\n\t"

    // Finished
    "y86_fin:" "\n\t"

        "movd %%mm3, %%eax" "\n\t"

        // Restore data
        "movd %%mm7, 28(%%esp)" "\n\t"
        "movd %%mm6, 24(%%esp)" "\n\t"
        "pushal" "\n\t"
        "movd %%mm1, 12(%%esp)" "\n\t"

        "movd %%mm0, %%esp" "\n\t"

        "popf" "\n\t"
        "popal"// "\n\t"
        :
        : "r" (&y->reg[0])
    );
}

void y86_trace_pc(Y_data *y) {
    Y_word index;
    Y_word diff;
    Y_word index_x;
    Y_word diff_x = Y_X_INST_SIZE;

    for (index = 0; index < Y_Y_INST_SIZE; ++index) {
        diff = y->reg[yr_rey] - (Y_word) y->x_map[index];

        // After step
        if (!diff) {
            index_x = index;
            break;
        }

        // After interrupt
        if (diff > 0 && diff < diff_x) {
            index_x = index;
            diff_x = diff;
        }
    }

    y->reg[yr_pc] = index_x;
}

Y_word y86_get_im_ptr() {
    Y_word result;
    __asm__ __volatile__("movd %%mm4, %0": "=r" (result));
    return result;
}

void y86_go(Y_data *y, Y_word step) {
    Y_word goon = 0;

    y86_ready(y, step);

    y86_trace_ip(y);
    do {
        y86_exec(y);

        switch (y->reg[yr_st]) {
            case ys_ima:
                y86_trace_pc(y);

                // Already failed
                y->reg[yr_pc] += 1;
                y->reg[yr_st] = ys_adr;

                goon = 0;
                break;

            case ys_imc:
                // y86_trace_pc(y);

                if (y86_get_im_ptr() + 4 < y->reg[yr_len]) {
                    y->reg[yr_len] = y86_get_im_ptr() + 4;
                }
                y86_load_all(y);

                y->reg[yr_st] = ys_aok;

                goon = 1;
                // y86_trace_ip(y);
                break;

            case ys_ret:
                // y86_trace_pc(y);

                // TODO: checking

                // Do return
                y->reg[yr_pc] = IO_WORD(&(y->mem[y->reg[yrl_esp]]));
                y->reg[yrl_esp] += 4;

                if (y->reg[yr_pc] >= y->reg[yr_len]) { // TODO: change this hack
                    y->reg[yr_sc] -= 2;
                    y->reg[yr_pc] += 1;

                    y->reg[yr_st] = ys_hlt;
                    goon = 0;
                    break;
                }

                y->reg[yr_st] = ys_aok;

                goon = 1;
                y86_trace_ip(y);
                break;

            default:
                y86_trace_pc(y);
                goon = 0;
                break;
        }
    } while (goon);
}

void y86_output_error(Y_data *y) {
    switch (y->reg[yr_st]) {
        case ys_adr:
            if (y->mem[y->reg[yr_pc] - 1] >= 0 /*< yi_call*/) { // Evil hack !? TODO
                fprintf(stdout, "PC = 0x%x, Invalid data address 0x%x\n", y->reg[yr_pc] - 1, y86_get_im_ptr());
            } else {
                fprintf(stdout, "PC = 0x%x, Invalid stack address 0x%x\n", y->reg[yr_pc] - 1, y86_get_im_ptr());
            }
            break;
        case ys_ins:
            fprintf(stdout, "PC = 0x%x, Invalid instruction %.2x\n", y->reg[yr_pc] - 1, y->mem[y->reg[yr_pc] - 1]);
            break;
        case ys_clf:
            fprintf(stdout, "PC = 0x%x, File loading failed\n", /*y->reg[yr_pc] - 1*/ 0);
            break;
        case ys_ccf:
            fprintf(stdout, "PC = 0x%x, Parsing or compiling failed\n", y->reg[yr_pc] - 1);
            break;
        case ys_adp:
            fprintf(stdout, "PC = 0x%x, Invalid instruction address\n", y->reg[yr_pc] - 1);
            break;
        case ys_inp:
            fprintf(stdout, "PC = 0x%x, Invalid instruction address\n", y->reg[yr_pc] - 1);
            break;
        /*case ys_mir:
            fprintf(stdout, "PC = 0x%x, Invalid instruction address on read\n", y->reg[yr_pc] - 1);
            break;
        case ys_miw:
            fprintf(stdout, "PC = 0x%x, Invalid instruction address on write\n", y->reg[yr_pc] - 1);
            break;*/
        default:
            break;
    }
}

Y_word y86_cc_transform(Y_word cc_x) {
    return ((cc_x >> 11) & 1) | ((cc_x >> 6) & 2) | ((cc_x >> 4) & 4);
}

void y86_output_state(Y_data *y) {
    const Y_char *stat_names[8] = {
        "AOK", "HLT", "ADR", "INS", "", "", "ADR", "INS"
    };

    const Y_char *cc_names[8] = {
        "Z=0 S=0 O=0",
        "Z=0 S=0 O=1",
        "Z=0 S=1 O=0",
        "Z=0 S=1 O=1",
        "Z=1 S=0 O=0",
        "Z=1 S=0 O=1",
        "Z=1 S=1 O=0",
        "Z=1 S=1 O=1"
    };

    fprintf(
        stdout,
        "Stopped in %d steps at PC = 0x%x.  Status '%s', CC %s\n",
        y->reg[yr_sx] - y->reg[yr_sc] - 1, y->reg[yr_pc] - !!y->reg[yr_st], stat_names[7 & y->reg[yr_st]], cc_names[y86_cc_transform(y->reg[yr_cc])]
    );
}

void y86_output_reg(Y_data *y) {
    Y_reg_lyt index;

    const Y_char *reg_names[yr_cnt] = {
        "%edi", "%esi", "%ebp", "%esp", "%ebx", "%edx", "%ecx", "%eax"
    };

    fprintf(stdout, "Changes to registers:\n");
    for (index = yr_cnt - 1; (Y_word) index >= 0; --index) {
        if (y->reg[index] != y->bak_reg[index]) {
            fprintf(stdout, "%s:\t0x%.8x\t0x%.8x\n", reg_names[index], y->bak_reg[index], y->reg[index]);
        }
    }
}

void y86_output_mem(Y_data *y) {
    Y_word index;

    fprintf(stdout, "Changes to memory:\n");
    for (index = 0; index < Y_MEM_SIZE; index += 4) { // Y_MEM_SIZE = 4 * n
        if (IO_WORD(&(y->bak_mem[index])) != IO_WORD(&(y->mem[index]))) {
            fprintf(stdout, "0x%.4x:\t0x%.8x\t0x%.8x\n", index, IO_WORD(&(y->bak_mem[index])), IO_WORD(&(y->mem[index])));
        }
    }
}

void y86_output(Y_data *y) {
    y86_output_error(y);
    y86_output_state(y);
    y86_output_reg(y);
    fprintf(stdout, "\n");
    y86_output_mem(y);
}

void y86_free(Y_data *y) {
    munmap(y, sizeof(Y_data));
}

void f_usage(Y_char *pname) {
    fprintf(stderr, "Usage: %s file.bin [max_steps]\n", pname);
}

Y_stat f_main(Y_char *fname, Y_word step) {
    Y_data *y = y86_new();
    Y_stat result;
    y->reg[yr_st] = setjmp(y->jmp);

    if (!(y->reg[yr_st])) {
        // Load
        if (strcmp(fname, "nil")) {
            y86_load_file(y, fname);
        } else {
            y->reg[yr_len] = 1;
            y->mem[0] = yi_halt;
        }

        y86_load_all(y);

        // Exec
        y86_go(y, step);
    } else {
        // Jumped out
    }

    // Output
    y86_output(y);

    // Return
    result = y->reg[yr_st];
    y86_free(y);
    return result == ys_clf || result == ys_ccf;
}

int main(int argc, char *argv[]) {
    switch (argc) {
        // Correct arg
        case 2:
            return f_main(argv[1], 10000);
        case 3:
            return f_main(argv[1], atoi(argv[2]));

        // Bad arg or no arg
        default:
            f_usage(argv[0]);
            return 0;
    }
}
