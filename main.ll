; ModuleID = 'Nucleus'
source_filename = "Nucleus"

define i32 @addTwo() {
entry:
  ret i32 2
}

define i32 @main() {
entry:
  %variableName = alloca i32, align 4
  store i32 2, ptr %variableName, align 4
  %variableName1 = load i32, ptr %variableName, align 4
  %addtmp = add i32 %variableName1, 2
  store i32 %addtmp, ptr %variableName, align 4
  %autoLoad = load i32, ptr %variableName, align 4
  ret i32 %autoLoad
}
