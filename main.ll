define i32 @main() {
entry:
  %variableName = alloca i32, align 4
  store i32 6, ptr %variableName, align 4
  %variableTwo = alloca i32, align 4
  store i32 8, ptr %variableTwo, align 4
  %variableName1 = load i32, ptr %variableName, align 4
  %addtmp = add i32 %variableName1, 8
  %subtmp = sub i32 %addtmp, 4
  store i32 %subtmp, ptr %variableName, align 4
  %variableTwo2 = load i32, ptr %variableTwo, align 4
  %variableName3 = load i32, ptr %variableName, align 4
  %addtmp4 = add i32 %variableTwo2, %variableName3
  store i32 %addtmp4, ptr %variableTwo, align 4
  %autoLoad = load i32, ptr %variableTwo, align 4
  ret i32 %autoLoad
}
