maybeOS is a simple 32 bit kernel meant to familiarize myself with OS development. As of right now, it is a purely kernel-space OS that launches a basic terminal, which supports command line editing and feeds input into a basic shell which suppports two commands currently. Future work is expanded upon below.

## Overview:
  - ISA: IA-32
  - Microarchitecture: i686
  - Firmware: EDK2 i386 UEFI firmware (BIOS not supported)    
  - Memory: up to 4GiB supported
  - Graphics Card: QEMU Standard VGA (others may or may not work)
  - IOAPIC: i82093AA (PIC is disabled by the kernel)
  - PS/2 Controller: i8042 (only the keyboard is utilized)
  - Bootloader: GRUB (grub-efi-ia32, 2.06-13; not grub or grub2) (but can be booted by any Multiboot2-compliant bootloader)

## Execution Environment:
  QEMU i386 (QEMU 9.0.1, installed with Homebrew)  
  Target Platform setup:
    
    qemu-system-i386 \
    -drive if=pflash,format=raw,unit=0,file=/opt/homebrew/Cellar/qemu/9.0.0/share/qemu/edk2-i386-code.fd,readonly=on \
    -drive if=pflash,format=raw,unit=1,file=/opt/homebrew/Cellar/qemu/9.0.0/share/qemu/edk2-i386-vars.fd \
    -hda <the GPT-partitioned raw disk holding the maybeOS.elf and bootloader in the ESP partition> \
    -serial file:qemu_serial.log \
    -monitor stdio

## Build Environment:
  
My true host is a 2020 M1 Mac running Sonoma 14.4.1; I found that setting up the compilation toolchain is easier on Linux, so I decided to do dev work by ssh into a Debian VM. After the maybeOS.elf image is compiled, it is transferred to my host to be copied into a raw disk and launched with another QEMU VM that emulates the targeted platform. The shared folder part of the Debain QEMU VM command is the transfer point for the image between my Mac host and the Debian VM.
  
OS Compiled On: Debian 6.1.90-1, with Linux kernel 6.1.0-21-amd64  
Development VM Setup:

    qemu-system-x86_64 \
    -m 2048 \
    -smp 4 \
    -hda <the Debian image in a QEMU QCOW2 Image format disk> \
    -net nic \
    -net user,hostfwd=tcp::2222-:22 \
    -virtfs local,security_model=mapped,mount_tag=shared,path= <desired mount point path on host>
  
  ### Toolchain:
  - Target: i686
  - Exe Format: ELF32 v1
    
  - Compiler: GNU GCC 12.2.0
      - Configuration:
        - <path to build dir for gcc>/gcc-12.2.0/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers
    
  - Assembler and Linker: GNU Binutils 2.39 (GNU as and GNU ld)
    - Configuration:
        - <path to build dir for binutils>/binutils-2.39/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
    
  - Installation:
      - Dependencies: The full list is available from the resources I followed to install this below
      - Environment Variables:
          - PREFIX=<desired path to install the cross-compiler at, so it doesn't install to the host system default compiler location>
          - TARGET=i686-elf
    
- Build Tools: GNU Make 4.3, bear 3.1.1 (bear only used to generate the compile_commands.json)
    
- Build commands used in the Makefile:
      - PREFIX/i686-elf-gcc <options> <src file>
      - PREFIX/i686-elf-as <options> <src file>
    
- Compiler/Assembler/Linker CLI options: All included in the Make in the root directory of this repo
   
- Resources For Installation of Cross Compiler/Assembler/Linker:
    - https://wiki.osdev.org/Cross-Compiler_Successful_Builds
    - https://wiki.osdev.org/GCC_Cross-Compiler

#### Commands for writing maybeOS.elf image to disk on Mac host:

      sudo hdiutil attach -nomount <GPT-partitioned raw disk image> &&
      sudo diskutil mountDisk <file in /dev representing the attached disk, with target partition specified> &&
      sudo cp maybeOS.elf <mount point on host/<desired path to OS image in the raw disk file system> > &&
      sudo diskutil eject <GPT-partitioned raw disk image>

## Summary of Features
- A Multiboot2 header makes maybeOS bootable by any Multiboot2-compliant bootloader
- Paging is enabled, and the kernel is moved to the higher half of virtual memory
- ACPI information is parsed to determine the available hardware and fetch its configurations
- Memory management is performed by three modules:
    - Physical Memory Manager
        - Utilizes a bitmap to track pages allocated
        - Attempts to allocate contiguous pages if possible utilizing a first fit algorithm
    - Virtual Memory Manager
        - Allocates virtual pages to requestees; if the address range is left to the VMM, it will also utilize a first fit algorithm
        - Creates PTEs in the specified paging structures
        - Tracks virtual allocations in a linked list of address range data structures
      - Kheap
        - Performs any-sized allocations (but is meant for < 1 page in size allocations), also utilizing the first fit algorithm
        - If possible, performs merging a freed area in the heap with its following neighbor
- IOAPIC and LAPIC are discovered and initialized
- ISRs are set up for keyboard interrupts; other ISRs are caught and have state info dumped to a serial port
- Drivers
    - PS/2 Controller
        - Converts set 2 scancodes and translates them into keycodes (using USB keycodes)
    - VGA framebuffer
        - Custom text mode implemented on top of the raw framebuffer
    - Serial port
        - Debug information is printed to the serial port
- Characters are rendered by printing a PSF font bitmap
- Various libc functions are implemented (mostly in string.h, some in ctype.h) as needed by referencing the ISO C documentation
- Terminal driver, which takes keycodes and translates them into a usable command or UI functionality
- Extremely basic shell, which parses a finalized command passed to it from the terminal driver, executes it, and passes it back to the terminal driver to have the output printed
