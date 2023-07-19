; ModuleID = 'nyb'
@N = global i32 -1, align 4
define i32 @main() {
entry:
  li t0, 1
  sd t0,0 ( sp )
  li t1, 2
  sd t1,4 ( sp )
  li t2, 33
  sd t2,8 ( sp )
  li t3, 4
  sd t3,12 ( sp )
  li t0, 5
  sd t0,16 ( sp )
  li t1, 6
  sd t1,20 ( sp )
  li t2, 0
  li t3, 0
  add t1, t3, zero
  add t0, t2, zero
  j %loopCondition
loopCondition:
  li t4, 6
  blt t0, t4, %loop
  j %exit
loop:
  li t5, 4
  mul t2, t5, t0
  add t3, sp, t2
  ld t4, 0 ( t3 )
  add t5, t1, t4
  addi t2, t0, 1
  add t1, t5, zero
  add t0, t2, zero
  j %loopCondition
exit:
  lowret t1
}
