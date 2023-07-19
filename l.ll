; ModuleID = 'nyb'
define i32 @whileIf() {
entry:
  li t0, 0
  add t2, t0, zero
  add t3, t1, zero
  li t1, 0
  j %loopCondition
loopCondition:
  t2 = phi  [ t0, %entry ], [ t6, %if_end ]
  t3 = phi  [ t1, %entry ], [ t1, %if_end ]
  li t4, 100
  blt t2, t4, %loop
  j %exit
loop:
  li t5, 5
  beq t2, t5, %if_then
  j %if_else
exit:
  lowret t3
if_then:
  li t0, 25
  add t1, t0, zero
  j %if_end
if_else:
  li t1, 10
  beq t2, t1, %if_then1
  j %if_else1
if_end:
  add t2, t6, zero
  add t3, t1, zero
  addi t6, t2, 1
  j %loopCondition
if_then1:
  li t2, 42
  j %if_end1
if_else1:
  li t4, 2
  add t2, t5, zero
  mul t5, t2, t4
  j %if_end1
if_end1:
  add t1, t2, zero
  j %if_end
}
define i32 @main() {
entry1:
  call whileIf
  add t0, a0, zero
  lowret t0
}
