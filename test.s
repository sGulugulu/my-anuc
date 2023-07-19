.global main
.data
.text
whileIf:
  addi sp, sp, -8
  sd ra, 0 ( sp ) 
entry: 
  li t0, 0
  add t2, t0, zero
  add t3, t1, zero
  li t1, 0
  j loopCondition
loopCondition: 
undef
undef
  li t4, 100
  blt t2, t4, loop
  j exit
loop: 
  li t5, 5
  beq t2, t5, if_then
  j if_else
if_then: 
  li t0, 25
  add t1, t0, zero
  j if_end
if_else: 
  li t1, 10
  beq t2, t1, if_then1
  j if_else1
if_end: 
  add t2, t6, zero
  add t3, t1, zero
  addi t6, t2, 1
  j loopCondition
if_then1: 
  li t2, 42
  j if_end1
if_else1: 
  li t4, 2
  add t2, t5, zero
  mul t5, t2, t4
  j if_end1
if_end1: 
  add t1, t2, zero
  j if_end
exit: 
  mv a0, t3
  ld ra, 0 ( sp ) 
  addi sp, sp, 8
  ret
main:
  addi sp, sp, -8
  sd ra, 0 ( sp ) 
entry1: 
  call whileIf
  add t0, a0, zero
  mv a0, t0
  ld ra, 0 ( sp ) 
  addi sp, sp, 8
  ret
