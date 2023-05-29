; ModuleID = 'nyb'
define i32 @main() {
entry:
  %sum = alloca i32, align 4
  %n = alloca i32, align 4
  %retval = alloca i32, align 4
  %x0 = load i32, ptr %n, align 4
  store i32 3, ptr %n, align 4
  %x1 = load i32, ptr %n, align 4
  store i32 5, ptr %n, align 4
  %x2 = load i32, ptr %sum, align 4
  %x3 = load i32, ptr %n, align 4
  store i32 %x3, ptr %sum, align 4
  %x4 = load i32, ptr %n, align 4
  store i32 7, ptr %n, align 4
  %x5 = load i32, ptr %sum, align 4
  store i32 %x5, ptr %retval, align 4
  %x6 = load i32, ptr %retval, align 4
  ret i32 %x6
}
