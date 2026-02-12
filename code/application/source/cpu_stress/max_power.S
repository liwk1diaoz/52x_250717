#ifndef ENTRY
#define ENTRY(name) \
        .globl name; \
name:
#endif

#ifndef WEAK
#define WEAK(name) \
        .weak name; \
name:
#endif

#ifndef END
#define END(name) \
        .size name, .-name
#endif

#define ENDPROC(name) \
        .type name, %function; \
        END(name)

.arch   armv7-a
.fpu    neon-fp-armv8


ENTRY(power_loop)
	stmfd       sp!, {r0-r12, lr}
	mov     r11, r0								// Get the total testing loop counts
	mov     r14, r1								// output memory buffer
	mov     r9,  #0x007
	mov     r10, #0x005
	mov     r12, #0x001
	ldr     r0, =0x00000000
	ldr     r1, =0xFFFFFFFD
	ldr     r2, =0xFFFFFFFC
	ldr     r3, =0x00000006
	ldr     r4, =0x00000000
	ldr     r5, =0xFFFFFFFD
	ldr     r6, =0xFFFFFFFC
	ldr     r7, =0x00000006
	vmov.i8 q1, #0x66  // q1 = 0x66666666
	vmov.i8 q2, #0xAA  // q2 = 0xAAAAAAAA
	vmov.i8 q3, #0x66  // q3 = 0x66666666
	vmov.i8 q4, #0xAA  // q4 = 0xAAAAAAAA
	vmov.i8 q5, #0x66  // q5 = 0x66666666
	vmov.i8 q6, #0xAA  // q6 = 0xAAAAAAAA
	vmov.i8 q7, #0x66  // q7 = 0x66666666
	vmov.i8 q8, #0xAA  // q8 = 0xAAAAAAAA
	vmov.i8 q9, #0x66  // q9 = 0x66666666
	#if 1
power_loop_sub:
	vfma.f32  q1, q2, q3           // Dual Issue Pair 0, Instruction 0
	//ldr       r8, [r0,+r1,LSL #1]! // Dual Issue Pair 0, Instruction 1
	//ldr       r8, [r2,+r3]!        // Dual Issue Pair 1, Instruction 0
	add       r0,r0,r1,LSL #1 // Dual Issue Pair 0, Instruction 1
	add       r2,r2,r3 // Dual Issue Pair 0, Instruction 1
	add       r0, r1, r9, LSR r12  // Dual Issue Pair 1, Instruction 1
	sub       r2, r3, r10, LSL r12 // Dual Issue Pair 2, Instruction 0
	//ldr       r8, [r4,+r5,LSL #1]! // Dual Issue Pair 2, Instruction 1
	//ldr       r8, [r6,+r7]         // Dual Issue Pair 3, Instruction 0
	add       r4,r4,r5,LSL #1 // Dual Issue Pair 0, Instruction 1
	add       r6,r6,r7 // Dual Issue Pair 0, Instruction 1
	add       r4, r5, r9, LSR r12  // Dual Issue Pair 3, Instruction 1
	vfma.f32  q4, q5, q6           // Dual Issue Pair 0, Instruction 0
	//ldr       r8, [r0,+r1,LSL #1]! // Dual Issue Pair 0, Instruction 1
	//ldr       r8, [r2,+r3]!        // Dual Issue Pair 1, Instruction 0
	add       r0,r0,r1,LSL #1 // Dual Issue Pair 0, Instruction 1
	add       r2,r2,r3 // Dual Issue Pair 0, Instruction 1
	add       r0, r1, r9, LSR r12  // Dual Issue Pair 1, Instruction 1
	sub       r2, r3, r10, LSL r12 // Dual Issue Pair 2, Instruction 0
	//ldr       r8, [r4,+r5,LSL #1]! // Dual Issue Pair 2, Instruction 1
	//ldr       r8, [r6,+r7]         // Dual Issue Pair 3, Instruction 0
	add       r4,r4,r5,LSL #1 // Dual Issue Pair 0, Instruction 1
	add       r6,r6,r7 // Dual Issue Pair 0, Instruction 1
	add       r4, r5, r9, LSR r12  // Dual Issue Pair 3, Instruction 1
	vfma.f32  q7, q8, q9           // Dual Issue Pair 0, Instruction 0
	//ldr       r8, [r0,+r1,LSL #1]! // Dual Issue Pair 0, Instruction 1
	//ldr       r8, [r2,+r3]!        // Dual Issue Pair 1, Instruction 0
	add       r0,r0,r1,LSL #1 // Dual Issue Pair 0, Instruction 1
	add       r2,r2,r3 // Dual Issue Pair 0, Instruction 1
	add       r0, r1, r9, LSR r12  // Dual Issue Pair 1, Instruction 1
	sub       r2, r3, r10, LSL r12 // Dual Issue Pair 2, Instruction 0
	//ldr       r8, [r4,+r5,LSL #1]! // Dual Issue Pair 2, Instruction 1
	//ldr       r8, [r6,+r7]         // Dual Issue Pair 3, Instruction 0
	add       r4,r4,r5,LSL #1 // Dual Issue Pair 0, Instruction 1
	add       r6,r6,r7 // Dual Issue Pair 0, Instruction 1
	add       r4, r5, r9, LSR r12  // Dual Issue Pair 3, Instruction 1
	vfma.f32  q4, q5, q6           // Dual Issue Pair 0, Instruction 0
	//ldr       r8, [r0,+r1,LSL #1]! // Dual Issue Pair 0, Instruction 1
	//ldr       r8, [r2,+r3]!        // Dual Issue Pair 1, Instruction 0
	add       r0,r0,r1,LSL #1 // Dual Issue Pair 0, Instruction 1
	add       r2,r2,r3 // Dual Issue Pair 0, Instruction 1
	add       r0, r1, r9, LSR r12  // Dual Issue Pair 1, Instruction 1
	sub       r2, r3, r10, LSL r12 // Dual Issue Pair 2, Instruction 0
	//ldr       r8, [r4,+r5,LSL #1]! // Dual Issue Pair 2, Instruction 1
	//ldr       r8, [r6,+r7]         // Dual Issue Pair 3, Instruction 0
	add       r4,r4,r5,LSL #1 // Dual Issue Pair 0, Instruction 1
	add       r6,r6,r7 // Dual Issue Pair 0, Instruction 1
	add       r4, r5, r9, LSR r12  // Dual Issue Pair 3, Instruction 1
	STR		r0, [r14]			// To store the value to output buffer.
	add		r14, r14, #4
	STR		r2, [r14]			// To store the value to output buffer.
	add		r14, r14, #4
	STR		r4, [r14]			// To store the value to output buffer.
	add		r14, r14, #4
	STR		r6, [r14]			// To store the value to output buffer.
	add		r14, r14, #4
	// End of loop check
	subs    r11, r11, #1
	bne     power_loop_sub
#endif
	ldmfd       sp!, {r0-r12, pc}
ENDPROC(power_loop)
