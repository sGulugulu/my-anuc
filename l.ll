; ModuleID = 'nyb'
define i32 @test() {
entry:
  %i = alloca i32, align 4
  %retval = alloca i32, align 4
  store i32 0, ptr %i, align 4
  br label %loopCondition
loopCondition:
  %x0 = load i32, ptr %i, align 4
  %x1 = icmp slt i32 %x0, 5
  br i1 %x1, label %loop, label %exit
loop:
  %x2 = load i32, ptr %i, align 4
  %x3 = load i32, ptr %i, align 4
  %x4 = add nsw i32 %x3, 1
  store i32 %x4, ptr %i, align 4
  br label %loopCondition
exit:
  %x5 = load i32, ptr %i, align 4
  store i32 %x5, ptr %retval, align 4
  %x6 = load i32, ptr %retval, align 4
  ret i32 %x6
}
