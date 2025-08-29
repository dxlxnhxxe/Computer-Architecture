(call$Test.demo)

// Save the current stack pointer in LCL (used for local segment)
@SP
D=M        // D = SP
@LCL
M=D        // LCL = SP

// Initialize local variables (LCL[0] = 0)
@SP
A=M        // A = SP
M=0        // *SP = 0
@SP
M=M+1      // SP++

// Initialize another local variable (LCL[1] = 0)
@SP
A=M        // A = SP
M=0        // *SP = 0
@SP
M=M+1      // SP++

// --- Push constant 7 to the stack ---
@7
D=A        // D = 7 (constant)

// Push D to the top of the stack
@SP
A=M        // A = SP
M=D        // *SP = 7
@SP
M=M+1      // SP++

// --- Pop value from stack into LCL[1] ---

// Decrement SP to access top value
@SP
M=M-1      // SP--

// Load value at top of stack into D
A=M        // A = SP
D=M        // D = *SP

// Store D into LCL[1]
@LCL
A=M        // A = LCL (base address of local segment)
A=A+1      // Move to LCL[1]
M=D        // LCL[1] = D (popped value)