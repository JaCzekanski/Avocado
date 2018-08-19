# Extension stub to boot .exe files during BIOS boot
# Sets breakpoint on GUI shell execution
# Works with breakpoint support enabled only

.set mips1
.set noreorder

.text
.globl _start 
_start:

.word post_boot
.ascii "Licensed by Sony Computer Entertainment Inc."
.rept 0x50
.byte 0
.endr

.word pre_boot 
.ascii "Licensed by Sony Computer Entertainment Inc."
.rept 0x50
.byte 0
.endr

pre_boot:
    li $t0, 0x80030000   # gui start address
    mtc0 $t0, $3         # BPC - breakpoint address, execution

    li $t0, (1<<24)      # enable breakpoint on GUI execution
    mtc0 $t0, $7         # DCIC breakpoint control

    jr $ra
    nop

# Will be called when breakpoint hit
post_boot:
    jr $ra
    nop
