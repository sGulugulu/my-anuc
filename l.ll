; ModuleID = 'nyb'
@n = global i32 0, align 4
define i32 @insertsort(ptr %argva) {
entry:
  %i = alloca i32, align 4
  store i32 1, ptr %i, align 4
  %x0 = load i32, ptr @n, align 4
  store i32 %x0, ptr %i, align 4
  %x1 = load i32, ptr %i, align 4
  %x2 = load i32, ptr @n, align 4
  %x3 = icmp slt i32 %x1, %x2
  br i1 %x3, label %if_then, label %if_end
if_then:
  %x4 = load i32, ptr %i, align 4
  %x5 = add i32 %x4, 1
  store i32 %x5, ptr %i, align 4
  br label %if_end
if_end:
  %i0 = phi i32 [ ] 
  %x6 = load i32, ptr %i, align 4
  ret i32 %x6
}
