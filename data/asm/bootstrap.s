.set mips1
.set noreorder

.text
.globl _start 
_start:

.word boot2 # post-boot entrypoint
.ascii "Licensed by Sony Computer Entertainment Inc."
.rept 0x50
.byte 0
.endr

.word boot1 # pre-boot entrypoint
.ascii "Licensed by Sony Computer Entertainment Inc."
.rept 0x50
.byte 0
.endr

boot1:
#	li  $t0, '1'
#	li   $t1, 0x1f802023
#	sb	 $t0, 0($t1) 

    li $t0, 0x80030000   # gui start address
    mtc0 $t0, $3         # BPC - breakpoint address, execution

    li $t0, (1<<24)      # enable breakpoint on GUI execution
    mtc0 $t0, $7         # DCIC breakpoint control

	jr $ra
	nop

boot2:
#	li   $t0, '2'
#	li   $t1, 0x1f802023
#	sb	 $t0, 0($t1) 

	li   $t0, 0x80010000
	jalr $t0
	nop

	.word  0xfc000000 #  invalid instruciton to break emulation
