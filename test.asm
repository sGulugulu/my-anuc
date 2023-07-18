.global main
.text
deepWhileBr:
entry: 
  add t0, a0, zero
  add t1, a1, zero
  add t2, t0, t1
  add t3, t2, zero
  j loopCondition
loopCondition: 
  li t1, 75
  blt t3, t1, loop
  j exit
loop: 
  li t2, 100
  blt t3, t2, if_then
  j phi_block
exit: 
  mv a0, t3
  ret
if_then: 
  addi t4, t3, 42
  li t5, 99
  bge t4, t5, if_then1
  j phi_block1
if_end: 
  add t3, t0, zero
  j loopCondition
phi_block: 
  add t0, t3, zero
  j if_end
if_then1: 
  li t0, 84
  j if_then2
if_end1: 
  add t0, t5, zero
  j if_end
phi_block1: 
  add t5, t4, zero
  j if_end1
if_then2: 
  li t1, 2
  add t3, t2, zero
  mul t2, t0, t1
  j if_end2
if_end2: 
  add t5, t3, zero
  j if_end1
phi_block2: 
  add t3, t4, zero
  j if_end2
main:
entry1: 
  li a0, 2
  li a1, 2
undef
  mv a0, t0
  ret
