@7
D=M
@500
M=D

@2
D=A
@501
M=D
D=M

@500
M=D-M
D=M
@label1
D;JEQ
@label2
D;JGT
@label3
D;JLT

(label1)
@5
D=A
@3
M=D
@END
0;JMP

(label2)
@7
D=M
@25
D=D+A
@3
M=D
@END
0;JMP

(label3)
@7
D=M
D=D+M
D=D+M
D=D+M
@3
M=D
@END
0;JMP

(END)
@END
0;JMP