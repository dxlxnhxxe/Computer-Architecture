@16384
D=A
@addr
M=D

@32
D=A
@counter
M=D

(LOOP)
@addr
A=M
M=-1

@addr
M=M+1     

@counter
M=M-1
D=M

@LOOP
D;JGT

@END
(END)
@END
0;JMP