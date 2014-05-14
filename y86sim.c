#include "y86sim.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
// #include <signal.h>
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
        *((Y_word *) y->x_end) = value;
        y->x_end += sizeof(Y_word);
    } else {
        fprintf(stderr, "Too large compiled instruction size (word: 0x%x)\n", value);
        longjmp(y->jmp, ys_ccf);
    }
}

void y86_push_x_addr(Y_data *y, Y_addr value) {
    if (y->x_end + sizeof(Y_addr) <= &(y->x_inst[Y_X_INST_SIZE])) {
        *((Y_addr *) y->x_end) = value;
        y->x_end += sizeof(Y_addr);
    } else {
        fprintf(stderr, "Too large compiled instruction size (addr: 0x%x)\n", (Y_word) value);
        longjmp(y->jmp, ys_ccf);
    }
}

void y86_link_x_map(Y_data *y, Y_size pos) {
    if (pos < Y_Y_INST_SIZE) {
        y->x_map[pos] = y->x_end;
        // fprintf(stderr, "%x---%x\n", pos, y->x_end);
    } else {
        fprintf(stderr, "Too large y86 instruction size\n");
        longjmp(y->jmp, ys_ccf);
    }
}

void y86_gen_before(Y_data *y) {
    YX(0x0F) YX(0x7E) YX(0xCC) // movd %mm1, %esp
}

void y86_gen_after(Y_data *y) {
    YX(0x0F) YX(0x6E) YX(0xCC) // movd %esp, %mm1
    YX(0x0F) YX(0x7E) YX(0xD4) // movd %mm2, %esp
    YX(0xFF) YX(0x14) YX(0x24) // call (%esp)
}

void y86_gen_stat(Y_data *y, Y_stat stat) {
    YX(0x0F) YX(0x6E) YX(0x3D) YXA((Y_addr) &(y_static_num[stat])) // movd stat, %mm6
}

void y86_gen_after_to(Y_data *y, Y_addr value) {
    // If changed, size should be updated (for jump instruction etc.)

    YX(0x0F) YX(0x6E) YX(0xCC) // movd %esp, %mm1
    YX(0x0F) YX(0x7E) YX(0xD4) // movd %mm2, %esp
    YX(0x68) YXA(value) // push value
    YX(0xFF) YX(0x64) YX(0x24) YX(0x04) // jmp 4(%esp)
}

Y_char y86_x_regbyte_C(Y_reg ra, Y_reg rb) {
    return 0xC0 | (ra >> 3) | rb;
}

Y_char y86_x_regbyte_8(Y_reg ra, Y_reg rb) {
    return 0x80 | (ra >> 3) | rb;
}

void y86_gen_protect(Y_data *y) {
    y86_gen_before(y);
    y86_gen_stat(y, ys_inp);
    y86_gen_after(y);
}

void y86_gen_x(Y_data *y, Y_inst op, Y_reg ra, Y_reg rb, Y_word val) {
    y86_gen_before(y);

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
                // TODO: check
                YX(0x89) YX(y86_x_regbyte_8(ra, rb)) // movl ...
                // if (rb == yr_esp) YX(0x24) // For esp
                YXW(val + (Y_word) &(y->mem[0]))
            } else {
                y86_gen_stat(y, ys_ins);
            }
            break;
        case yi_mrmovl:
            if (ra < yr_cnt && rb < yr_cnt) {
                // TODO: check
                YX(0x8B) YX(y86_x_regbyte_8(ra, rb)) // movl ...
                // if (rb == yr_esp) YX(0x24) // For esp
                YXW(val + (Y_word) &(y->mem[0]))
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
                if (y->x_map[val]) {
                    switch (op) {
                        case yi_jmp:
                            break;
                        case yi_jle:
                            YX(0x7F) YX(0x0F) // jg +15
                            break;
                        case yi_jl:
                            YX(0x7D) YX(0x0F) // jge +15
                            break;
                        case yi_je:
                            YX(0x75) YX(0x0F) // jne +15
                            break;
                        case yi_jne:
                            YX(0x74) YX(0x0F) // je +15
                            break;
                        case yi_jge:
                            YX(0x7C) YX(0x0F) // jl +15
                            break;
                        case yi_jg:
                            YX(0x7E) YX(0x0F) // jle +15
                            break;
                        default:
                            // Impossible
                            break;
                    }
                    y86_gen_after_to(y, y->x_map[val]);
                } else {
                    y86_gen_stat(y, ys_inp);
                }
            } else {
                y86_gen_stat(y, ys_adp);
            }
            break;
        case yi_call:
            if (val >= 0 && val < Y_Y_INST_SIZE) {
                if (y->x_map[val]) {
                    // TODO:
                    y86_gen_after_to(y, y->x_map[val]);
                } else {
                    y86_gen_stat(y, ys_inp);
                }
            } else {
                y86_gen_stat(y, ys_adp);
            }
            break;
        case yi_ret:
            // TODO: check esp
            // pop val
            // jmp x_map[val]
            break;
        case yi_pushl:
            //
            break;
        case yi_popl:
            //
            break;
        case yi_bad:
            y86_gen_stat(y, ys_ins);
            break;
        default:
            // Impossible
            break;
    }

    y86_gen_after(y);
}

void y86_parse(Y_data *y, Y_char *begin, Y_char **inst, Y_char *end) {
    y->reg[yr_pc] = *inst - begin;
    y86_link_x_map(y, y->reg[yr_pc]);

    Y_inst op = **inst;
    (*inst)++;

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
            val = *((Y_word *) *inst);
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
            val = *((Y_word *) *inst);
            *inst += sizeof(Y_word);

            break;
        case yi_bad:
        default:
            op = yi_bad;

            break;
    }

    y86_gen_x(y, op, ra, rb, val);
}

void y86_load(Y_data *y) {
    Y_char *begin = &(y->y_inst[0]);
    Y_char *inst = begin;
    Y_char *end = begin + y->reg[yr_len];

    y->x_end = &(y->x_inst[0]);
    y->reg[yr_pc] = 0;

    while (inst != end) {
        y86_parse(y, begin, &inst, end);
    }
    y86_link_x_map(y, y->reg[yr_pc] + 1);
    y86_gen_protect(y);

    y->reg[yr_pc] = 0;
}

void y86_load_file_bin(Y_data *y, FILE *binfile) {
    clearerr(binfile);

    y->reg[yr_len] = fread(&(y->y_inst[0]), sizeof(Y_char), Y_Y_INST_SIZE, binfile);
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

void y86_debug_exec(Y_data *y) {
    y86_gen_x(y, yi_halt, yr_nil, yr_nil, 0);
    y->reg[yr_st] = ys_hlt;
}

void y86_ready(Y_data *y, Y_word step) {
    y->reg[yr_cc] = 0x40;
    y->reg[yr_rey] = (Y_word) y->x_map[y->reg[yr_pc]];
    // printf("%x, %x", y->reg[yr_pc], y->x_map[0]);
    y->reg[yr_sx] = step;
    y->reg[yr_sc] = step;
    y->reg[yr_st] = ys_aok;
}

void __attribute__ ((noinline)) y86_exec(Y_data *y) {
    __asm__ __volatile__ (
        "pushal" "\n\t"
        "pushfd" "\n\t"

        "movd %%esp, %%mm0" "\n\t"
        "movl %0, %%esp" "\n\t"

        // Load data
        "movd 12(%%esp), %%mm1" "\n\t"
        "popal" "\n\t"

        "movd 16(%%esp), %%mm4" "\n\t"
        "movd 20(%%esp), %%mm5" "\n\t"
        "movd 24(%%esp), %%mm6" "\n\t"
        "movd 28(%%esp), %%mm7" "\n\t"

        // Build callback stack
        "addl $12, %%esp" "\n\t"
        "pushl $y86_call" "\n\t"
        "movd %%esp, %%mm2" "\n\t"

        "subl $8, %%esp" "\n\t"
        "popfd" "\n\t"

        // Call
        "y86_call:" "\n\t"

        "pushfd" "\n\t"
        "movd %%eax, %%mm3" "\n\t"

        // Check step
        "movd %%mm6, %%eax" "\n\t"
        "decl %%eax" "\n\t"
        "movd %%eax, %%mm6" "\n\t"
        "testl %%eax, %%eax" "\n\t"
        "js y86_fin" "\n\t"

        // Check state
        "movd %%mm7, %%eax" "\n\t"
        "testl %%eax, %%eax" "\n\t"
        "jnz y86_fin" "\n\t"

        "movd %%mm3, %%eax" "\n\t"
        "popfd" "\n\t"

        "ret" "\n\t"

        // Finished
        "y86_fin:" "\n\t"

        "movd %%mm3, %%eax" "\n\t"

        // Restore data
        "movd %%mm4, 16(%%esp)" "\n\t"
        "movd %%mm5, 20(%%esp)" "\n\t"
        "movd %%mm6, 24(%%esp)" "\n\t"
        "movd %%mm7, 28(%%esp)" "\n\t"

        "pushal" "\n\t"
        "movd %%mm1, 12(%%esp)" "\n\t"

        "movd %%mm0, %%esp" "\n\t"

        "popfd" "\n\t"
        "popal"// "\n\t"
        :
        : "r" (&y->reg[0])
    );
}

void y86_trace_pc(Y_data *y) {
    Y_size index;
    for (index = 0; index < Y_Y_INST_SIZE; ++index) {
        if (y->reg[yr_rey] == (Y_word) y->x_map[index]) {
            y->reg[yr_pc] = index;
            break;
        }
    }
    if (index == Y_Y_INST_SIZE) {
        fprintf(stderr, "Unknown instruction pointer at 0x%x (PC = 0x%x, X[PC] = 0x%x)\n", y->reg[yr_rey], y->reg[yr_pc], (Y_word) y->x_map[y->reg[yr_pc]]);
        longjmp(y->jmp, ys_inp);
    }
}

void y86_output_error(Y_data *y) {
    switch (y->reg[yr_st]) {
        case ys_adr:
            fprintf(stdout, "PC = 0x%x, Invalid instruction address\n", y->reg[yr_pc]);
            break;
        case ys_ins:
            fprintf(stdout, "PC = 0x%x, Invalid instruction\n", y->reg[yr_pc]);
            break;
        case ys_clf:
            fprintf(stdout, "PC = 0x%x, File loading failed\n", /*y->reg[yr_pc]*/ 0);
            break;
        case ys_ccf:
            fprintf(stdout, "PC = 0x%x, Parsing or compiling failed\n", y->reg[yr_pc]);
            break;
        case ys_adp:
            fprintf(stdout, "PC = 0x%x, Invalid instruction address detected staticly\n", y->reg[yr_pc]);
            break;
        case ys_inp:
            fprintf(stdout, "PC = 0x%x, Invalid instruction detected staticly\n", y->reg[yr_pc]);
            break;
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
    Y_reg index;

    const Y_char *reg_names[yr_cnt] = {
        "%edi", "%esi", "%ebp", "%esp", "%ebx", "%edx", "%ecx", "%eax"
    };

    fprintf(stdout, "Changes to registers:\n");
    for (index = yr_cnt - 1; (Y_word) index >= 0; --index) {
        if (y->reg[index]) {
            fprintf(stdout, "%s:\t0x%.8x\t0x%.8x\n", reg_names[index], 0, y->reg[index]);
        }
    }
}

void y86_output_mem(Y_data *y) {
    Y_size index;

    fprintf(stdout, "Changes to memory:\n");
    for (index = 0; index < Y_MEM_SIZE; ++index) {
        if (y->mem[index]) {
            fprintf(stdout, "0x%.4x:\t0x%.8x\t0x%.8x\n", index, 0, y->mem[index]);
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

/*
void f_signal() {
    fprintf(stderr, "aaa\n");
}

void f_signal_init() {
    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = f_signal;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);
    sigaction(SIGSEGV, &sigIntHandler, NULL);
}*/

void f_usage(Y_char *pname) {
    fprintf(stderr, "Usage: %s file.bin [max_steps]\n", pname);
}

Y_stat f_main(Y_char *fname, Y_word step) {
    // f_signal_init();

    Y_data *y = y86_new();
    Y_stat result;
    y->reg[yr_st] = setjmp(y->jmp);

    if (!(y->reg[yr_st])) {
        // Load
        if (strcmp(fname, "nil")) {
            y86_load_file(y, fname);
            y86_load(y);
        }

        // Exec
        #ifdef Y_DEBUG
        y86_debug_exec(y);
        #endif

        y86_ready(y, step);
        y86_exec(y);
        y86_trace_pc(y);
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
