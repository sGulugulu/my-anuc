; ModuleID = 'nyb'
define i32 @main() {
entry:
  lowstore 0, %a, offset: 0
  lowload %x2, %a, offset: 79996
  ret i32 %x2
}
