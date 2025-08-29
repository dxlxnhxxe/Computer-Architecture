@R0
D=M

@R1
M=D

(label1)
@R8
M=A
D=D-M
@R2
M=D
@END
D;JLT
@R1
M=D
@label1
D;JGT
@END

(END)
@END
0;JMP

