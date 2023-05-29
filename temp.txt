; ModuleID = 'nyb'
define i32 @test() {
entry:
  br label %loopCondition
loopCondition:
  %i0 = phi i32 [ 0, %entry ], [ %x4, %loop ]
  %x1 = icmp slt i32 %i0, 5
  br i1 %x1, label %loop, label %exit
loop:
  %x4 = add nsw i32 %i0, 1
  br label %loopCondition
exit:
  ret i32 %x5
}
