define i32 @main() {
entry:
  %variableName = alloca i32, align 4
  store i32 0, ptr %variableName, align 4
  %variableTwo = alloca i32, align 4
  store i32 6, ptr %variableTwo, align 4
  %variableName1 = load i32, ptr %variableName, align 4
  %addtmp = add i32 %variableName1, 2
  %addtmp2 = add i32 %addtmp, 4
  %addtmp3 = add i32 %addtmp2, 6
  %addtmp4 = add i32 %addtmp3, 8
  %variableTwo5 = load i32, ptr %variableTwo, align 4
  %addtmp6 = add i32 %variableTwo5, 6
  store i32 %addtmp4, ptr %variableName, align 4
  store i32 %addtmp6, ptr %variableTwo, align 4
  %autoLoad = load i32, ptr %variableTwo, align 4
  ret i32 %autoLoad
}
