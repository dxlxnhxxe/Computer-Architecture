@7
D=M         // Load RAM[7] into D
@500
M=D         // Store D into RAM[500]

@2
D=A         // D = 2
@501
M=D         // Store D into RAM[501]

@500
D=M         // D = RAM[500]
@501
D=D-M       // D = RAM[500] - RAM[501]

@label1
D;JEQ       // If D == 0, jump to label1
@label2
D;JGT       // If D > 0, jump to label2
@label3
D;JLT       // If D < 0, jump to label3

(label1)
@5
D=A         // D = 5
@3
M=D         // Store 5 in RAM[3]
@END
0;JMP       // Jump to END

(label2)
@7
D=M         // Load RAM[7] into D
@25
D=D+A       // D = D + 25
@3
M=D         // Store result in RAM[3]
@END
0;JMP       // Jump to END

(label3)
@7
D=M         // Load RAM[7] into D
D=D+M       // D = D + RAM[7] (now D = 2 * RAM[7])
D=D+M       // D = D + RAM[7] (now D = 3 * RAM[7])
D=D+M       // D = D + RAM[7] (now D = 4 * RAM[7])
@3
M=D         // Store result in RAM[3]
@END
0;JMP       // Jump to END

(END)
@END
0;JMP       // Infinite loop to stop the program