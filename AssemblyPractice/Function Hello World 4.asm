// Translation of:
// function Hello.world 4
// push temp 0
// pop local 2

(call$Hello.world)
// Function Hello.world 4 - Initialize 4 local variables to 0
// Push 0 onto stack 4 times and pop into local 0,1,2,3

// Initialize local 0 = 0
@SP
A=M
M=0
@SP
M=M+1

@SP
A=M-1
D=M
@SP
M=M-1
@LCL
A=M
M=D

// Initialize local 1 = 0
@SP
A=M
M=0
@SP
M=M+1

@SP
A=M-1
D=M
@SP
M=M-1
@LCL
A=M+1
M=D

// Initialize local 2 = 0
@SP
A=M
M=0
@SP
M=M+1

@SP
A=M-1
D=M
@SP
M=M-1
@LCL
A=M+1
A=A+1
M=D

// Initialize local 3 = 0
@SP
A=M
M=0
@SP
M=M+1

@SP
A=M-1
D=M
@SP
M=M-1
@LCL
A=M+1
A=A+1
A=A+1
M=D

// push temp 0
// temp segment starts at RAM[5], so temp 0 is RAM[5]
@5
D=M
@SP
A=M
M=D
@SP
M=M+1

// Pop from stack and store in local[2]
@SP
A=M-1
D=M
@SP
M=M-1
@LCL
A=M+1
A=A+1
M=D