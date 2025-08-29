// Load the value stored at the address pointed to by R0
@R0
A=M        // A = RAM[R0]
D=M        // D = *RAM[R0]

// If D > 0, jump to label2
@label2
D;JGT

(label2)
@R0
A=M        // A = RAM[R0]
D=M        // D = *RAM[R0]

// Compute two's complement of D and store in R2
@R2
M=!D       // R2 = bitwise NOT of D
M=M+1      // R2 = -D (two's complement)
D=M        // D = R2

// If D (i.e., -original_value) < 0 (original_value > 0), jump to label1
@label1
D;JLT

// If D == 0 (original_value == 0), jump to END
@END
D;JEQ

(label1)
// Increment R0 to point to the next address
@R0
M=M+1

// Load the new address from R0 into A
A=M        // A = RAM[R0]

// Increment R1 (could be used as a counter or result tracker)
@R1
M=M+1

// Unconditional indirect jump to the address stored in R0
@R0
(label3)
M;JMP      // Jump to address in RAM[R0]

(END)
// Infinite loop to halt the program
@END
0;JMP