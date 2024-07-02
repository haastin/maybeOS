/*
#cant use C for these because a compiler will add in its own stack handling instructions, but the interrupt handlers use
#a diff stack frame format
*/


interrupt_handler_skeleton:
pusha
#because we are using System V ABI for IA-32, all params will be passed on the stack
push %esp //the address where the process_context_t struct data begins 
call interrupt_handler_dispatcher
popa
add $16, %esp #get rid of the vector number and real or fake error code on the stack
iret
 

.macro interrupt_handler vector has_error_code
.align 16
/*
we add a fake error code to the stack if there is not one so that we can always assume the stack frame for the
interrupt handler is the same size, because the return point to the interrupted program would otherwise sometimes 
pop an error code instead since not all interrupts/exceptions push error codes */
.if \has_error_code == 0 
        push 0xDEADBEEF
    .endif

#we will use another function to multiplex the proper C interrupt handler; we just pass the handler vector desired 
push \vector
jmp interrupt_handler_skeleton
.endm

#vectors_with_errorcodes: .byte 0x8, 0xA, 0xB, 0xC, 0xD, 0xE, 0x11, 0x15

.set num_arch_defined_interrupts, 32
.set num_user_defined_interrupts, 256 - num_arch_defined_interrupts
.set i, 32

#build the arch-defined interrupt vector handlers here. these are handwritten because some have error codes and some dont.
.global arch_defined_interrupts_start
arch_defined_interrupts_start:
vector_0:
    interrupt_handler 0 0
vector_1:
    interrupt_handler 1 0
vector_2:
    interrupt_handler 2 0
vector_3:
    interrupt_handler 3 0
vector_4:
    interrupt_handler 4 0
vector_5:
    interrupt_handler 5 0
vector_6:
    interrupt_handler 6 0
vector_7:
    interrupt_handler 7 0
vector_8:
    interrupt_handler 8 1
vector_9:
    interrupt_handler 9 0
vector_10:
    interrupt_handler 10 1
vector_11:
    interrupt_handler 11 1
vector_12:
    interrupt_handler 12 1
vector_13:
    interrupt_handler 13 1
vector_14:
    interrupt_handler 14 1
vector_15:
    interrupt_handler 15 0
vector_16:
    interrupt_handler 16 0
vector_17:
    interrupt_handler 17 1
vector_18:
    interrupt_handler 18 0
vector_19:
    interrupt_handler 19 0
vector_20:
    interrupt_handler 20 0
vector_21:
    interrupt_handler 21 1
vector_22:
    interrupt_handler 22 0
vector_23:
    interrupt_handler 23 0
vector_24:
    interrupt_handler 24 0
vector_25:
    interrupt_handler 25 0
vector_26:
    interrupt_handler 26 0
vector_27:
    interrupt_handler 27 0
vector_28:
    interrupt_handler 28 0
vector_29:
    interrupt_handler 29 0
vector_30:
    interrupt_handler 30 0
vector_31:
    interrupt_handler 31 0


#build user-defined interrupt vectors here
.rept num_user_defined_interrupts
interrupt_handler i 0
.set i, i+1
.endr


