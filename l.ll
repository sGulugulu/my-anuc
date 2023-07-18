; ModuleID = 'nyb'
@n = global i32 0, align 4
define i32 @insertsort(ptr %argva, i32 %argvi) {
entry:
  li %x6, 4
  mul %x5, %x6, %argvi
  addi %x7, %x5, 0
  add %x8, %argva, %x7
  lw %x3, 0 ( %x8 )
  lowret %x3
}
