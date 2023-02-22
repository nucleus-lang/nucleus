define i32 @main() {
entry:
  %variableName = alloca i32, align 4
  store i32 0, ptr %variableName, align 4
  %variableName1 = load i32, ptr %variableName, align 4
  %addtmp = add i32 %variableName1, 1
  store i32 %addtmp, ptr %variableName, align 4
  %state1 = load i32, ptr %variableName, align 4
  %addtmp2 = add i32 %state1, 1
  %state2 = load i32, ptr %variableName, align 4
  %addtmp3 = add i32 %state2, 2
  %state3 = load i32, ptr %variableName, align 4
  %addtmp4 = add i32 %state3, 3
  ret i32 %addtmp2
}
