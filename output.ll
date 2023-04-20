; ModuleID = 'Nucleus'
source_filename = "Nucleus"

define i32 @add_two_with(i32 %number) {
entry:
  %addtmp = add i32 %number, 2
  ret i32 %addtmp
}

define i32 @main() {
entry:
  %calltmp = call i32 @add_two_with(i32 2)
  ret i32 %calltmp
}
