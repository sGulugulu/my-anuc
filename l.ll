; ModuleID = 'nyb'
define i32 @main() {
entry:
  li %x28, -2
  li %x29, 2
  div %x2, %x28, %x29
  li %x30, 0
  add %x6, %x2, %x30
  li %x31, 4
  subi %x9, %x31, 0
  li %x32, 2
  rem %x10, %x9, %x32
  sub %x11, %x6, %x10
  li %x33, 0
  addi %x14, %x33, 67
  li %x34, 0
  subi %x18, %x34, 0
  add %x19, %x14, %x18
  li %x35, 3
  li %x36, 2
  rem %x22, %x35, %x36
  subi %x23, %x22, 0
  sub %x24, %x19, %x23
  addi %x26, %x24, 3
  lowret 0
}
