#include "y86sim.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>

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
    } else {
        fprintf(stderr, "Too large y86 instruction size\n");
        longjmp(y->jmp, ys_ccf);
    }
}

void y86_gen_save_esp(Y_data *y) {
    //y86_push_x(y, 0x89);
    //y86_push_x(y, 0x25);
    //y86_push_x_addr(y, &(y->ret));
}

void y86_gen_load_esp(Y_data *y) {
    //y86_push_x(y, 0x8B);
    //y86_push_x(y, 0x25);
    //y86_push_x_addr(y, &(y->ret));
}

void y86_gen_init(Y_data *y) {
    y86_gen_save_esp(y);
}

/*void y86_gen_call(Y_data *y, Y_addr target) {
    y86_gen_load_esp(y);
    //target-&(y->x_inst[y->x_end])-sizeof(y_addr)
    y86_gen_save_esp(y);
}*/

void y86_gen_ret(Y_data *y) {
    y86_gen_load_esp(y);
    y86_push_x(y, 0xC3);
}

void y86_gen_reg_update(Y_data *y, Y_reg r) {}

void y86_gen_step(Y_data *y) {}

void y86_gen_zf(Y_data *y) {}

void y86_gen_sf(Y_data *y) {}

void y86_gen_of(Y_data *y) {}

void y86_gen_pos(Y_data *y) {}

void y86_gen_stat(Y_data *y) {}

void y86_gen_x(Y_data *y, Y_inst op, Y_reg ra, Y_reg rb, Y_word val) {
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
        default:
            break;
    }
}

void y86_parse(Y_data *y, Y_char *begin, Y_char **inst, Y_char *end) {
    y86_link_x_map(y, *inst - begin);
    y->reg[yr_pc] = *inst - begin;

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

    while (inst != end) y86_parse(y, begin, &inst, end);

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

void y86_debug_exec(Y_data *y) {
    y86_gen_x(y, yi_halt, yr_nil, yr_nil, 0);
    y->reg[yr_st] = ys_hlt;
}

void y86_ready(Y_data *y, Y_word step) {
    y->reg[yr_sx] = step;
    y->reg[yr_sc] = 0;
    y->reg[yr_st] = ys_aok;
}

void y86_exec(Y_data *y) {
    ((Y_func) y->x_inst)();
}

void y86_free(Y_data *y) {
    munmap(y, sizeof(Y_data));
}

void f_usage(Y_char *pname) {
    #ifdef Y_RECORD_REG
    fprintf(stderr, "Usage: %s file.bin [max_steps]\n", pname);
    #else
    fprintf(stderr, "Usage: %s file.bin\n", pname);
    #endif
}

Y_word f_main(Y_char *fname, Y_word step) {
    Y_data *y = y86_new();
    Y_word result;
    Y_size index;
    y->reg[yr_st] = setjmp(y->jmp);

    if (!(y->reg[yr_st])) {
        // Load
        y86_load_file(y, fname);
        y86_load(y);

        // Exec
        #ifdef Y_DEBUG
        y86_debug_exec(y);
        #endif

        y86_ready(y, step);
        y86_exec(y);
    } else {
        // Jumped out
    }

    // Output
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

    const char *stat_names[4] = {
        "AOK", "HLT", "ADR", "INS"
    };
    const char *cc_names[8] = {
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
        y->reg[yr_sc], y->reg[yr_pc], stat_names[y->reg[yr_st]], cc_names[y->reg[yr_cc]]
    );


    const char *reg_names[yr_cnt] = {
        "%eax", "%ecx", "%edx", "%ebx", "%esp", "%ebp", "%esi", "%edi"
    };

    fprintf(stdout, "Changes to registers:\n");
    for (index = 0; index < yr_cnt; ++index) {
        if (y->reg[index]) {
            fprintf(stdout, "%s:\t0x%.8x\t0x%.8x\n", reg_names[index], 0, y->reg[index]);
        }
    }

    fprintf(stdout, "\n");
    fprintf(stdout, "Changes to memory:\n");
    for (index = 0; index < Y_MEM_SIZE; ++index) {
        if (y->mem[index]) {
            fprintf(stdout, "0x%.4x:\t0x%.8x\t0x%.8x\n", index, 0, y->mem[index]);
        }
    }


    // Return
    result = y->reg[yr_st];
    y86_free(y);
    return result != ys_hlt;
}

int main(int argc, char *argv[]) {
    switch (argc) {
        // Correct arg
        case 2:
            return f_main(argv[1], 10000);

        #ifdef Y_RECORD_REG
        case 3:
            return f_main(argv[1], atoi(argv[2]));
        #endif

        // Bad arg or no arg
        default:
            f_usage(argv[0]);
            return 0;
    }
}
