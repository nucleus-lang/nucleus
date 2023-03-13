; ModuleID = 'Nucleus'
source_filename = "Nucleus"

define i32 @add_two(i32 %test) {
entry:
  ret i32 2
}

define i32 @main() {
entry:
  %variable_name = alloca i32, align 4
  store i32 1, ptr %variable_name, align 4
  %variable_two = alloca i32, align 4
  store i32 2, ptr %variable_two, align 4
  %variable_name1 = load i32, ptr %variable_name, align 4
  %addtmp = add i32 %variable_name1, 3
  %addtmp2 = add i32 %addtmp, 3
  %addtmp3 = add i32 %addtmp2, 3
  store i32 %addtmp3, ptr %variable_name, align 4
  %variable_two4 = load i32, ptr %variable_two, align 4
  %variable_name5 = load i32, ptr %variable_name, align 4
  %addtmp6 = add i32 %variable_two4, %variable_name5
  store i32 %addtmp6, ptr %variable_two, align 4
  %autoLoad = load i32, ptr %variable_two, align 4
  ret i32 %autoLoad
}
