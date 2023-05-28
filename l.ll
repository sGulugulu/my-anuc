; ModuleID = 'nyb'
define i32 @Fibon1(i32 %argvn) {
entry:
  %n = alloca i32, align 4
  store i32 %argvn, ptr %n, align 4
  %retval = alloca i32, align 4
  %x0 = load i32, ptr %n, align 4
  %x1 = icmp eq i32  %x0, 1
  br  %x1, label %if_then, label %lor_lhs_false
if_then:
  store i32 1, ptr %retval, align 4
  br label %if_end
if_else:
  %x4 = load i32, ptr %n, align 4
  %x5 = sub nsw i32 %x4, 1
  br label %if_end
if_end:
  %x6 = load i32, ptr %retval, align 4
  ret i32 %x6
lor_lhs_false:
  %x2 = load i32, ptr %n, align 4
  %x3 = icmp eq i32  %x2, 2
  br  %x3, label %if_then, label %if_else
}
