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
    lea stack_top, %eax
    sub $_kernel_vm_offset, %eax
    mov %eax, %esp

    //push the address of the multiboot tags we get from GRUB here so we don't lose them before we parse them; will eventually be the parameter to kernel_start
    push %ebx

    call init_gdt

    lea _kernel_vm_start, %eax
    pushl %eax
    lea _kernel_phys_start, %eax
    pushl %eax

    call before_paging_page_dir_and_table_setup

    //remove the parameters we pushed to stack
    add $16, %esp

    #set root kernel page dir in cr3
    lea kernel_init_page_dir, %eax
    sub $_kernel_vm_offset, %eax
    mov %eax, %cr3

    #disable PAE
    mov %cr4, %eax
    and $~(1 << 5), %eax
    mov %eax, %cr4

    #enable paging
    mov %cr0, %eax
    or $(1 << 31), %eax
    mov %eax, %cr0
    #jmp $0x8,$.continue
    
    .continue:
    add $_kernel_vm_offset, %esp
   
    call kernel_start

    cli
    loop:
        hlt
        jmp loop

