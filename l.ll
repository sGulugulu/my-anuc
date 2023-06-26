; ModuleID = 'nyb'
@a = global i32 0, align 4
define i32 @main() {
entry:
  %c = alloca i32, align 4
  %retval = alloca i32, align 4
  %x0 = load i32, ptr @a, align 4
  store i32 %x0, ptr %c, align 4
  store i32 2, ptr @a, align 4
  %x1 = load i32, ptr %c, align 4
  store i32 %x1, ptr %retval, align 4
  %x2 = load i32, ptr %retval, align 4
  ret i32 %x2
}
