/*sets some constants used to construct our multiboot header so that maybeOS will conform to multiboot specification*/
.set ALIGN, 1<<0 /* 0 indicates the index of this var in the flags bitmap, same for meminfo below*/
.set MEMINFO, 1<<1
.set FLAGS, ALIGN | MEMINFO #constructs the bitmap
.set MAGIC, 0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

#actually writes these values to the output file
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

#allocating stack in bss saves space in this file's representation in the fs
.section .bss 
.align 16 #System V ABI expects the stack to be 16 byte aligned
stack_top: #this will be at a lower address since it is higher in the file, which is why it's the bottom
.skip 16384
stack_bottom:

.intel_syntax noprefix

.section .text
.global _start #chatGPT says convention is to use .global first
.type _start, @function
_start:

    /*at this point we have a code and data segment in a basic GDT passed from GRUB, but not our own GDT,
and paging isn't enabled*/
    mov esp, stack_top

    //we finish setting up in C now that our stack has been initialized
    call kernel_start

