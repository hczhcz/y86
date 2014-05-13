#include "y86sim.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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

void y86_gen_before(Y_data *y) {
    y86_push_x(y, 0x0F);
    y86_push_x(y, 0x7E);
    y86_push_x(y, 0xCC);
}

void y86_gen_after(Y_data *y) {
    y86_push_x(y, 0x0F);
    y86_push_x(y, 0x6E);
    y86_push_x(y, 0xCC);

    y86_push_x(y, 0x0F);
    y86_push_x(y, 0x7E);
    y86_push_x(y, 0xD4);

    y86_push_x(y, 0xFF);
    y86_push_x(y, 0x14);
    y86_push_x(y, 0x24);
}

void y86_gen_x(Y_data *y, Y_inst op, Y_reg ra, Y_reg rb, Y_word val) {
    y86_gen_before(y);

    switch (op) {
        case yi_halt:
            //
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

    y86_gen_after(y);
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
    y86_gen_x(y, yi_halt, yr_nil, yr_nil, 0);

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
    const char *stat_names[8] = {
        "AOK", "HLT", "ADR", "INS", "", "", "ADR", "INS"
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
        y->reg[yr_sx] - y->reg[yr_sc], y->reg[yr_pc], stat_names[y->reg[yr_st]], cc_names[y86_cc_transform(y->reg[yr_cc])]
    );
}

void y86_output_reg(Y_data *y) {
    Y_word index;

    const char *reg_names[yr_cnt] = {
        "%edi", "%esi", "%ebp", "%esp", "%ebx", "%edx", "%ecx", "%eax"
    };

    fprintf(stdout, "Changes to registers:\n");
    for (index = yr_cnt - 1; index >= 0; --index) {
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

void y86_new(Y_data *y) {
    mmap(
        0, sizeof(Y_data),
        PROT_READ | PROT_WRITE | PROT_EXEC,
        MAP_ANONYMOUS,
        -1, 0
    );

    y->x_end = &(y->x_inst[0]);
}

void y86_debug_exec(Y_data *y) {
    y86_gen_x(y, yi_halt, yr_nil, yr_nil, 0);
    y->reg[yr_st] = ys_hlt;
}

void y86_ready(Y_data *y, Y_word step) {
    y->reg[yr_cc] = 0;
    y->reg[yr_sx] = step;
    y->reg[yr_sc] = step;
    y->reg[yr_st] = ys_aok;
}

void __attribute__ ((noinline)) y86_exec(Y_data *y) {
    // printf("%x\n", y->reg);
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
        "pushl %%esp" "\n\t"
        "addl $24, (%%esp)" "\n\t"

        "subl $4, %%esp" "\n\t"
        "popfd" "\n\t"

        // Call
        "y86_call:" "\n\t"

        "movd %%eax, %%mm3" "\n\t"
        "pushfd" "\n\t"

        // Check state
        "movd %%mm7, %%eax" "\n\t"
        "testl %%eax, %%eax" "\n\t"
        "jz y86_fin" "\n\t"

        // Check step
        "movd %%mm6, %%eax" "\n\t"
        "decl %%eax" "\n\t"
        "movd %%eax, %%mm6" "\n\t"
        "testl %%eax, %%eax" "\n\t"
        "jz y86_fin" "\n\t"

        "popfd" "\n\t"
        "movd %%mm3, %%eax" "\n\t"

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

Y_word f_main(Y_char *fname, Y_word step) {
    Y_data mem_y;
    Y_data *y = &mem_y;
    y86_new(y);
    Y_word result;
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
    } else {
        // Jumped out
    }

    // Output
    y86_output(y);

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
        case 3:
            return f_main(argv[1], atoi(argv[2]));

        // Bad arg or no arg
        default:
            f_usage(argv[0]);
            return 0;
    }
}
