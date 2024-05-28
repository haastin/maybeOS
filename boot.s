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





