//Push return address
@RETURN_LABEL_1
D=A
@SP
A=M
M=D
@SP
M=M+1

//Push LCL
@LCL
D=M
@SP
A=M
M=D
@SP
M=M+1

//Push ARG
@ARG
D=M
@SP
A=M
M=D
@SP
M=M+1

//Push THIS
@THIS
D=M
@SP
A=M
M=D
@SP
M=M+1

//Push THAT
@THAT
D=M
@SP
A=M
M=D
@SP
M=M+1

//Set ARG = SP - 5 - nArgs (nArgs = 2)
@SP
D=M
@7
D=D-A
@ARG
M=D


//Set LCL = SP
@SP
D=M
@LCL
M=D

//Jump to math.multiply
@Math.multiply
0;JMP

//Return label
(RETURN_LABEL_1)