; ModuleID = 'Nucleus'
source_filename = "Nucleus"

define i32 @addTwo() {
entry:
  ret i32 2
}

define i32 @main() {
entry:
  %variableName = alloca i32, align 4
  store i32 1, ptr %variableName, align 4
  %State1 = load i32, ptr %variableName, align 4
  %addtmp = add i32 %State1, 1
  %State2 = load i32, ptr %variableName, align 4
  %addtmp1 = add i32 %State2, 2
  %addtmp2 = add i32 %addtmp1, %addtmp
  %State3 = load i32, ptr %variableName, align 4
  %addtmp3 = add i32 %State3, 3
  %addtmp4 = add i32 %addtmp1, %addtmp2
  %addtmp5 = add i32 %addtmp3, %addtmp4
  ret i32 %addtmp5
}