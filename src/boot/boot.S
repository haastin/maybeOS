#define ASM_FILE
#include "multiboot2.h"

#implementing a multiboot2 header to be booted by GRUB

#GRUB can apparently ignore the length field if the checksum is still correct; seems like the length field is irrelvant if checksum is correct. There also NEEDS to be a terminating tag even if no OS-specified tags are passed to GRUB, because, again, GRUB ignores the length field and looks for that instead apparently.
.set MULTIBOOT2_HEADER_LENGTH, .multiboot2_header_end - .multiboot2_header_start 
.set MULTIBOOT2_HEADER_CHECKSUM, -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT2_ARCHITECTURE_I386 + MULTIBOOT2_HEADER_LENGTH)


.section .multiboot2
.align MULTIBOOT_HEADER_ALIGN
.multiboot2_header_start:
.long MULTIBOOT2_HEADER_MAGIC
.long MULTIBOOT2_ARCHITECTURE_I386
.long MULTIBOOT2_HEADER_LENGTH
.long MULTIBOOT2_HEADER_CHECKSUM

.terminating_tag_start:
.short 0 #type
.short 0 #flags
.long 8 #size
.terminating_tag_end:

.multiboot2_header_end:

#allocating stack in bss saves space in this file's representation in the fs
.section .bss 
.align 16 #System V ABI expects the stack to be 16 byte aligned
stack_bottom: #this will be at a lower address since it is higher in the file, which is why it's the bottom
.skip 16384
stack_top:

.section .text
.global _start #chatGPT says convention is to use .global first
.type _start, @function
_start:

    /*at this point we have a code and data segment in a basic GDT passed from GRUB, but not our own GDT,
and paging isn't enabled*/
    mov $stack_top, %esp
    //we finish setting up in C now that our stack has been initialized
   
    call kernel_start

    cli
    loop:
        hlt
        jmp loop

