// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.

// Avoid being marked as needing an executable stack:
#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif

// Further contents are loongarch64 only:
#if defined(__linux__) && defined(__loongarch64)

.section .text
.globl internal_sigreturn
.type internal_sigreturn, @function
internal_sigreturn:

        li.d $r11,139 // #139 is for SYS_rt_sigreturn
        syscall 0

.size internal_sigreturn, .-internal_sigreturn

#endif // defined(__linux__) && defined(__loongarch64)
