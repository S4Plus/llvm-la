.section .text

.hidden __tsan_setjmp
.comm _ZN14__interception11real_setjmpE,8,8
.globl setjmp
.type setjmp, @function
setjmp:

  # save env parameters
  addi.d $r3,$r3,-24
  st.d $r1,$r3,16
  st.d $r22,$r3,8

  # save jmp_buf
  st.d $r4,$r3,0

  # obtain $sp
  add.d $r4,$r0,$r3

  # call tsan interceptor
  addi.d $r5,$r4,24
  bl __tsan_setjmp

  # restore jmp_buf
  ld.d $r4,$r3,0

  # load libc setjmp to r20
  la $r20,_ZN14__interception11real_setjmpE
  # restore env parameters
  ld.d $r22,$r3,8
  ld.d $r1,$r3,16
  addi.d $r3,$r3,24

  # tail jump to libc setjmp
  ld.d $r20,$r20,0
  jr $r20

.size setjmp, .-setjmp

.hidden __tsan_setjmp
.globl _setjmp
.comm _ZN14__interception12real__setjmpE,8,8
.type _setjmp, @function
_setjmp:

  # Save env parameters
  addi.d $r3,$r3,-24
  st.d $r1,$r3,16
  st.d $r22,$r3,8

  # save jmp_buf
  st.d $r4,$r3,0

  # obtain $sp
  add.d $r4,$r0,$r3

  # call tsan interceptor
  addi.d $r5,$r4,24
  bl __tsan_setjmp

  # restore jmp_buf
  ld.d $r4,$r3,0

  # load libc _setjmp to r20
  la $r20,_ZN14__interception12real__setjmpE

  # restore env parameters
  ld.d $r22,$r3,8
  ld.d $r1,$r3,16
  addi.d $r3,$r3,24

  # tail jump to libc _setjmp
  ld.d $r20,$r20,0
  jr $r20

.size _setjmp, .-_setjmp

.hidden __tsan_setjmp
.globl sigsetjmp
.comm _ZN14__interception14real_sigsetjmpE,8,8
.type sigsetjmp, @function
sigsetjmp:

  # Save env parameters
  addi.d $r3,$r3,-32
  st.d $r1,$r3,24
  st.d $r22,$r3,16

  # save jmp_buf and savesig
  st.d $r4,$r3,0
  st.d $r5,$r3,8

  # obtain $sp
  add.d $r4,$r0,$r3

  # call tsan interceptor
  addi.d $r5,$r4,32
  bl __tsan_setjmp

  # restore jmp_buf and savesig
  ld.d $r4,$r3,0
  ld.d $r5,$r3,8

  # load libc sigsetjmp to r20
  la $r20,_ZN14__interception14real_sigsetjmpE

  # restore env parameters
  ld.d $r22,$r3,16
  ld.d $r1,$r3,24
  addi.d $r3,$r3,32

  # tail jump to libc sigsetjmp
  ld.d $r20,$r20,0
  jr $r20

.size sigsetjmp, .-sigsetjmp

.hidden __tsan_setjmp
.comm _ZN14__interception16real___sigsetjmpE,8,8
.globl __sigsetjmp
.type __sigsetjmp, @function
__sigsetjmp:

  # Save env parameters
  addi.d $sp,$sp,-32
  st.d $r1,$r3,24
  st.d $r22,$r3,16

  # save jmp_buf and savesig
  st.d $r4,$r3,0
  st.d $r5,$r3,8

  # obtain $sp
  add.d $r4,$r0,$r3

  # call tsan interceptor
  addi.d $r5,$r4,32
  bl __tsan_setjmp

  # restore jmp_buf and savesig
  ld.d $r4,$r3,0
  ld.d $r5,$r3,8

  # load libc __sigsetjmp in r20
  la $r20,_ZN14__interception16real___sigsetjmpE

  # restore env parameters
  ld.d $r22,$r3,16
  ld.d $r1,$r3,24
  addi.d $r3,$r3,32

  # tail jump to libc __sigsetjmp
  ld.d $r20,$r20,0
  jr $r20

.size __sigsetjmp, .-__sigsetjmp
