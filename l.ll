; ModuleID = 'nyb'
define i32 @main() {
entry:
  %x1 = mul i32 -2, 1
  %x2 = sdiv i32 %x1, 2
  %x5 = sub i32 5, 5
  %x6 = add i32 %x2, %x5
  %x8 = add i32 1, 3
  %x9 = sub i32 0, %x8
  %x10 = srem i32 %x9, 2
  %x11 = sub i32 %x6, %x10
  %x13 = srem i32 -2, 2
  %x14 = add i32 %x13, 67
  %x17 = sub i32 5, 5
  %x18 = sub i32 0, %x17
  %x19 = add i32 %x14, %x18
  %x21 = add i32 1, 2
  %x22 = srem i32 %x21, 2
  %x23 = sub i32 0, %x22
  %x24 = sub i32 %x19, %x23
  %x26 = add i32 %x24, 3
  ret i32 0
}
