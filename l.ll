; ModuleID = 'nyb'
define i32 @deepWhileBr(i32 %argva, i32 %argvb) {
entry:
  add t0, a0, a1
  add t1, t0, zero
  ja %loopCondition
loopCondition:
  li t3, 75
  blt t1, t3, %loop
  ja %exit
loop:
  li t3, 100
  blt t1, t3, %if_then
  ja %if_end
exit:
  lowret t1
if_then:
  addi t1, t1, 42
  li t1, 99
  bge t1, t1, %if_then1
  ja %if_end1
if_end:
  ja %loopCondition
if_then1:
  li t1, 84
  ja %if_then2
if_end1:
  ja %if_end
if_then2:
  li t1, 2
  mul t3, t1, t1
  add t1, t3, zero
  ja %if_end2
if_end2:
  ja %if_end1
}
define i32 @main() {
entry:
  %x20 = call i32 @deepWhileBr(i32 2, i32 2)
  lowret %x20
}
