.global main
.data
N:
  .word -1
.text
main:
  addi sp, sp, -8
  sd ra, 0 ( sp ) 
  addi sp, sp, -400
entry: 
  li t0, 1
  sw t0, 0 ( sp )
  li t1, 2
  sw t1, 4 ( sp )
  li t2, 33
  sw t2, 8 ( sp )
  li t3, 4
  sw t3, 12 ( sp )
  li t0, 5
  sw t0, 16 ( sp )
  li t1, 6
  sw t1, 20 ( sp )
  li t2, 0
  li t3, 0
  add t1, t3, zero
  add t0, t2, zero
  j loopCondition
loopCondition: 
  li t4, 6
  blt t0, t4, loop
  j exit
loop: 
  li t5, 4
  mul t2, t5, t0
  add t3, sp, t2
  lw t4, 0 ( t3 )
  add t5, t1, t4
  addi t2, t0, 1
  add t1, t5, zero
  add t0, t2, zero
  j loopCondition
exit: 
  mv a0, t1
  addi sp, sp, 400
  ld ra, 0 ( sp ) 
  addi sp, sp, 8
  ret
