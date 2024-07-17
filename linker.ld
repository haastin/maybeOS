/* the symbol from the symbol table (generated from all .o files) defined to be the start of execution of the OS image*/
ENTRY(_start)
 
/* Maps object file sections to memory addresses */
SECTIONS
{
	/* OSdev wiki recommendeds the OS image be located at physical mem address 2 MB based on research around best compatability with UEFI firmware and bootloader memory mappings. */

	_kernel_vm_start = 0xC0200000;
	_kernel_phys_start = 0x00200000;
	_kernel_vm_offset = 0xC0000000;


	/*certain functions are linked into physical memory addresses so that we can use them before paging is enabled */
	
	. = _kernel_phys_start;

	.pre_paging ALIGN(4K) : {
		_phys_kernel_start = .;
		*(.multiboot2)
		*boot.o(.text)
		*gdt.o(.text)
		*early_paging_setup.o(.text)

		#because the two above files contain all functions needed to enable mem access and paging, i can get rid of the custom section below and all attributes i placed with it in my src code
		*(.kernel_before_paging_code)
		_phys_kernel_end = .;
	}


	/*We want the OS image to be linked AS IF it starts from its virtual address, but we dont *actually* want it to be loaded there. we want it to be loaded in lower physical mem in an ideal spot to avoid overwriting anything already placed in RAM by prior executed code before us. Linking witht he assumption of the virtual addy start is one way to allow the kernel to be remapped at this address in the virtual address space after paging is enabled */
	
	. += _kernel_vm_offset;
 
	_virt_kernel_start = .;
	
	_text_start = .;
	.text ALIGN(4K) : AT(ADDR(.text) - _kernel_vm_offset) 
	{
		/* First put the multiboot header, as it is required to be put very early
	   in the image or the bootloader won't recognize the file format.
	   Next we'll put the .text section. */
		*(.text)
	}
	_text_end = .;

	

	_rodata_start = .;
	/* Read-only data. */
	.rodata ALIGN(4K) : AT(ADDR(.rodata) - _kernel_vm_offset) 
	{
		*(.rodata)
	}
	_rodata_end = .;

	_data_start = .;
	/* Read-write data (initialized) */
	.data ALIGN(4K) : AT(ADDR(.data) - _kernel_vm_offset) 
	{
		*(.data)
	}
	_data_end = .;

	_bss_start = .;
	/* Read-write data (uninitialized) and stack */
	.bss ALIGN(4K) : AT(ADDR(.bss) - _kernel_vm_offset) 
	{
		*(COMMON)
		*(.bss)
	}
	_bss_end = .;

	_virt_kernel_end = .;

	_kernel_phys_size = _phys_kernel_end - _phys_kernel_start;
	_kernel_virt_size = _virt_kernel_end - _virt_kernel_start ;

	_kernel_size = _kernel_phys_size + _kernel_virt_size;
}