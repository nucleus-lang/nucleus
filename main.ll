define i32 @main() {
entry:
  %variableName = alloca i32, align 4
  store i32 0, ptr %variableName, align 4
  %ref = load i32, ptr %variableName, align 4
  %addtmp = add i32 %ref, 2
  %addtmp1 = add i32 %addtmp, 6
  store i32 %addtmp1, ptr %variableName, align 4
  %autoLoad = load i32, ptr %variableName, align 4
  ret i32 %autoLoad
}
