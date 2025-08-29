@R0
D=M


@R1
M=D

(label1)
D=D-1
@R1
M=M+D

@label1
D;JGT
@END
0;JEQ


(END)
@END
0;JMP
