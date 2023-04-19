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
  %one = alloca i32, align 4
  store i32 %calltmp, ptr %one, align 4
  %autoLoad = load i32, ptr %one, align 4
  ret i32 %autoLoad
}
