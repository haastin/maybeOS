
/**normally, my understanding is that a tty subsystem would have the keyboard driver place raw keycodes into a buffer at a specific virtual device in a filesystem's buffer, and the that tty device would send these to the terminal emulator which would interpret these keycodes. however, for now I will fold this functionality into the kernel itself until a proper filesystem, etc. is set up; this way i can keep my console API the same and only move my calls from the kernel to a userspace terminal emulator when i get my fs, etc. set up*/

#define KEYCODE_BUFF_SIZE 255

//TODO: statically allocate or no?
unsigned short keycode_buff[KEYCODE_BUFF_SIZE];



