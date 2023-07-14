; ModuleID = 'nyb'
define i32 @main() {
entry:
  li %x28, -2
  sw %x28,48( sp)
  li %x29, 2
  sw %x29,52( sp)
  lw s1, 48 ( sp )
  lw s0, 52 ( sp )
  div %x2, s1, s0
  sw %x2,0( sp)
  li %x30, 0
  sw %x30,56( sp)
  lw s1, 0 ( sp )
  lw s0, 56 ( sp )
  add %x6, s1, s0
  sw %x6,4( sp)
  li %x31, 4
  sw %x31,60( sp)
  lw s0, 60 ( sp )
  subi %x9, s0, 0
  sw %x9,8( sp)
  li %x32, 2
  sw %x32,64( sp)
  lw s1, 8 ( sp )
  lw s0, 64 ( sp )
  rem %x10, s1, s0
  sw %x10,12( sp)
  lw s1, 4 ( sp )
  lw s0, 12 ( sp )
  sub %x11, s1, s0
  sw %x11,16( sp)
  li %x33, 0
  sw %x33,68( sp)
  lw s0, 68 ( sp )
  addi %x14, s0, 67
  sw %x14,20( sp)
  li %x34, 0
  sw %x34,72( sp)
  lw s0, 72 ( sp )
  subi %x18, s0, 0
  sw %x18,24( sp)
  lw s1, 20 ( sp )
  lw s0, 24 ( sp )
  add %x19, s1, s0
  sw %x19,28( sp)
  li %x35, 3
  sw %x35,76( sp)
  li %x36, 2
  sw %x36,80( sp)
  lw s1, 76 ( sp )
  lw s0, 80 ( sp )
  rem %x22, s1, s0
  sw %x22,32( sp)
  lw s0, 32 ( sp )
  subi %x23, s0, 0
  sw %x23,36( sp)
  lw s1, 28 ( sp )
  lw s0, 36 ( sp )
  sub %x24, s1, s0
  sw %x24,40( sp)
  lw s0, 40 ( sp )
  addi %x26, s0, 3
  sw %x26,44( sp)
  lowret 0
}
