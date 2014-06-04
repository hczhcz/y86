Yet another Y86 implementation
===

About Y86
---

Y86 is a simplified x86-like instruction set.

8 user-accessible registers:

`EAX ECX EDX EBX ESP EBP ESI EDI`

4 states:

`AOK HLT ADR INS`

Instructions:

`halt nop rr/cmovXX irmovl rmmovl mrmovl OPTl jXX call ret pushl popl`

ASM file example:

    # Execution begins at address 0
           .pos 0
    init:  irmovl Stack, %esp       # Set up stack pointer
           irmovl Stack, %ebp       # Set up base pointer
           call Main                # Execute main program
           halt                     # Terminate program

    # Array of 4 elements
           .align 4
    array: .long 0xd
           .long 0xc0
           .long 0xb00
           .long 0xa000

    Main:  pushl %ebp
           rrmovl %esp,%ebp
           irmovl $4,%eax
           pushl %eax               # Push 4
           irmovl array,%edx
           pushl %edx               # Push array
           call Sum                 # Sum(array, 4)
           rrmovl %ebp,%esp
           popl %ebp
           ret

    #/* $begin sum-ys 0 */
    # int Sum(int *Start, int Count)
    Sum:   pushl %ebp
           rrmovl %esp,%ebp
           mrmovl 8(%ebp),%ecx      # ecx = Start
           mrmovl 12(%ebp),%edx     # edx = Count
           xorl %eax,%eax           # sum = 0
           andl %edx,%edx           # Set condition codes
           je End
    Loop:  mrmovl (%ecx),%esi       # get *Start
           addl %esi,%eax           # add to sum
           irmovl $4,%ebx
           addl %ebx,%ecx           # Start++
           irmovl $-1,%ebx
           addl %ebx,%edx           # Count--
           jne Loop                 # Stop when 0
    End:   rrmovl %ebp,%esp
           popl %ebp
           ret
    #/* $end sum-ys 0 */

    # The stack starts here and grows to lower addresses
           .pos 0x100
    Stack:

For more details: Chapter 4 of [CSAPP](http://csapp.cs.cmu.edu/)

Y86 Simulator
---

This is a 'lab' (homework) in SE101/ICS (Introduction to Comupter Systems).

The original version is a 'pure' simulator, but I made a JIT compiler.

Build:

`cc -m32 -o y86sim y86sim.c` (recommended cc: clang)

Run:

`y86sim file.bin [max_steps]`

The 'max' version looks like the normal one, but there is no step counting and non-static error detecting in it.

Y86 Assembler
---

This is also a 'lab' in SE101/ICS.

The implementation is based on the original framework in the course.

Build:

`cc -m32 -o y86asm y86asm.c` (recommended cc: clang)

Run:

`y86asm [-v] file.ys`

`-v` print the readable output to screen.

License
---

The simulator is released under WTFPL. Keep calm and have fun with it :)

The others are not released under any license, so they can be used in education purpose only.

DO NOT CHEAT! The code is stored in the anti-cheating code base. You know.
