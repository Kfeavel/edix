.set MAGIC, 0x1badb002
.set FLAGS, (1<<0 | 1<<1)
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot
    .long MAGIC
    .long FLAGS
    .long CHECKSUM


.section .text
.extern kernelMain
.extern callConstructors
.global loader


loader:
    mov $kernel_stack, %esp
    call callConstructors
    push %eax
    push %ebx
    call kernelMain

# When the kernel is done we move down to stop, so we disable interrupts
# and then halt the CPU
_stop:
    cli
    hlt
    jmp _stop


.section .bss
.space 4*1024*1024; # 2 MiB
kernel_stack:

