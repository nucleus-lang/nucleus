; ModuleID = 'Nucleus'
source_filename = "Nucleus"

define i32 @main() {
entry:
  %variable = alloca i32, align 4
  store i32 2, ptr %variable, align 4
  %variable1 = load i32, ptr %variable, align 4
  %addtmp = add i32 %variable1, 4
  %addtmp2 = add i32 %addtmp, 6
  %addtmp3 = add i32 %addtmp2, 10
  store i32 %addtmp3, ptr %variable, align 4
  %autoLoad = load i32, ptr %variable, align 4
  ret i32 %autoLoad
}
