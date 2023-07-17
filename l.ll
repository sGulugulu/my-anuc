; ModuleID = 'nyb'
define i32 @deepWhileBr(i32 %argva, i32 %argvb) {
entry:
  add t1, t0, zero
  add t0, a0, a1
  j %loopCondition
loopCondition:
  li t2, 75
  blt t1, t2, %loop
  j %exit
loop:
  li t3, 100
  blt t1, t3, %if_then
  j %phi_block
exit:
  lowret t1
if_then:
  addi t4, t1, 42
  li t5, 99
  bge t4, t5, %if_then1
  j %phi_block1
if_end:
  add t1, t6, zero
  j %loopCondition
if_then1:
  li t6, 84
  j %if_then2
if_end1:
  add t6, t5, zero
  j %if_end
if_then2:
  add t3, t2, zero
  li t0, 2
  mul t2, t6, t0
  j %if_end2
if_end2:
  add t5, t3, zero
  j %if_end1
phi_block:
  add t6, t1, zero
  j %if_end
phi_block1:
  add t5, t4, zero
  j %if_end1
phi_block2:
  add t3, t4, zero
  j %if_end2
}
