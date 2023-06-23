# Assembler-project-Course-20465-
A simple assembler implementation for imaginary IA-32 like assembly language (excluding linker & loader)
Since this is a very simple assemly language and there will be no need to predict memory usage, I chose to implement a one-pass assembler.
With efficiency both in terms of run time and space complexity in mind, the chosen data structures to implement the assembler are queues and hash tables.
We will use two queues, one for directions and one for instructions and three hash tables, for extern, entry and a third for the symbol table.
After validating the file (given through cli),
  - We first start the macro expansion pass, if there's any error during this stage the program will not continue to further stages and move onto the next assembly file(if given) or exit.
      - During this pass, the program writes the newly expanded assembly onto a different file(with the same file name given at first but with a '.as' file extention) if no errors were found.
  - After the macro expansion pass we move onto the main pass where we will parse the expanded assembly data and extract all possible data needed for later stages.
  - We move to a macro pass to validate and fulfill missing information the extracted data during the main pass
  - At last if no errors were found until now, we move to the writing to files, else we exit. 

------------------------------ brief explanation ------------------------------


Background details:
The processor works with integer values only (Z) using two's complement, it also supports ascii characters.
We have 8 GPR, r0 to r7 each 14 bits of size and the lsb starts from the rhs.
The assembler will encode the machine instructions into a unique base 2 - / := 1  and . := 0 .

Instruction encoding:

+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
|  13 |  12 |  11 |  10 |  9  |  8  |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
|-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
|   param1  |   param2  |         opcode        | src addr  | dest addr | attributes|
+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
      (a)        (b)               (c)               (d)         (e)         (f) 


(a)(b) These bits are for direct register addressing mode 
        the bits of the corresponding parameter will be set to;
         - 00 for an immidiate value
         - 11 for a register
         - 01 for a symbol

(c) Represents the decimal value of an operation in binary 

+--------+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
|op      |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  10 |  11 |  12 |  13 |  14 |  15 |
|--------|-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
|opcode  | mov | cmp | add | sub | not | clr | lea | inc | dec | jmp | bne | red | prn | jsr | rts | stop|
+--------+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+

(d) Represents the addressing mode of the source operand

(e) Represents the addressing mode of the destination operand

(f) Represets the relocation attributes / flags
        - 00 for Absolute(fixed)
        - 01 for External(resolved by the linker)
        - 10 for Relocatable(modifiable)


Addressing modes:

  - Immediate addressing                                 ; mov#-1,r2
  - Direct addressing                                    ; x: .data 23
                                                              dec x
  - Jump addressing(with optional parameters)            ; jmp L1(#5,N)
                                                           jsr L1(r3,r5)
  - Direct register addressing                           ; mov r1,r2
