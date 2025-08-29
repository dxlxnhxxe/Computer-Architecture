// Push 7 onto the stack
@7       // Load 7 into A
D=A      // Store 7 in D
@SP      // Point to SP
A=M      // Set A to the value of SP (stack pointer)
M=D      // Store 7 at RAM[SP]
@SP      // Increment SP
M=M+1
// Push 8 onto the stack
@8       // Load 8 into A
D=A      // Store 8 in D
@SP      // Point to SP
A=M      // Set A to the value of SP (stack pointer)
M=D      // Store 8 at RAM[SP]
@SP      // Increment SP
M=M+1
// Add the top two elements of the stack
@SP      // Decrement SP (point to top of stack, value 8)
M=M-1
A=M      // Set A to RAM[SP]
D=M      // Load the value at RAM[SP] (8) into D
@SP      // Decrement SP (point to next stack value, 7)
M=M-1
A=M      // Set A to RAM[SP]
M=M+D    // Add the top two stack values (7 + 8) and store in RAM[SP]
// Final state: SP points to the correct stack position
@SP      // Increment SP to point to next free stack slot
M=M+1