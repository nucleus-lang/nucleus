; ModuleID = 'Nucleus'
source_filename = "Nucleus"

define i64 @main() {
entry:
  %0 = load i32, i32 2, align 4
  %addtmp = add i32 %0, 1
  %addtmp1 = add i32 %addtmp, 2
  ret i32 %addtmp1
}
