; ModuleID = 'Nucleus'
source_filename = "Nucleus"

define i32 @addTwo() {
entry:
  ret i32 2
}

define i32 @main() {
entry:
  %variableName = alloca i32, align 4
  store i32 0, ptr %variableName, align 4
  %variableName1 = load i32, ptr %variableName, align 4
  %addtmp = add i32 %variableName1, 1
  store i32 %addtmp, ptr %variableName, align 4
  %State1 = load i32, ptr %variableName, align 4
  %variableName2 = load i32, ptr %variableName, align 4
  %addtmp3 = add i32 %variableName2, 1
  store i32 %addtmp3, ptr %variableName, align 4
  %State2 = load i32, ptr %variableName, align 4
  %variableName4 = load i32, ptr %variableName, align 4
  %addtmp5 = add i32 %variableName4, 1
  store i32 %addtmp5, ptr %variableName, align 4
  %State3 = load i32, ptr %variableName, align 4
  %variableName6 = load i32, ptr %variableName, align 4
  %addtmp7 = add i32 %variableName6, 1
  store i32 %addtmp7, ptr %variableName, align 4
  %State4 = load i32, ptr %variableName, align 4
  %variableName8 = load i32, ptr %variableName, align 4
  %addtmp9 = add i32 %variableName8, 1
  store i32 %addtmp9, ptr %variableName, align 4
  %State5 = load i32, ptr %variableName, align 4
  %variableName10 = load i32, ptr %variableName, align 4
  %addtmp11 = add i32 %variableName10, 1
  store i32 %addtmp11, ptr %variableName, align 4
  %State6 = load i32, ptr %variableName, align 4
  %variableName12 = load i32, ptr %variableName, align 4
  %addtmp13 = add i32 %variableName12, 1
  store i32 %addtmp13, ptr %variableName, align 4
  %State7 = load i32, ptr %variableName, align 4
  %variableName14 = load i32, ptr %variableName, align 4
  %addtmp15 = add i32 %variableName14, 1
  store i32 %addtmp15, ptr %variableName, align 4
  %State8 = load i32, ptr %variableName, align 4
  %variableName16 = load i32, ptr %variableName, align 4
  %addtmp17 = add i32 %variableName16, 1
  store i32 %addtmp17, ptr %variableName, align 4
  %State9 = load i32, ptr %variableName, align 4
  %variableName18 = load i32, ptr %variableName, align 4
  %addtmp19 = add i32 %variableName18, 1
  store i32 %addtmp19, ptr %variableName, align 4
  %State10 = load i32, ptr %variableName, align 4
  %variableName20 = load i32, ptr %variableName, align 4
  %addtmp21 = add i32 %variableName20, 1
  store i32 %addtmp21, ptr %variableName, align 4
  %State11 = load i32, ptr %variableName, align 4
  %variableName22 = load i32, ptr %variableName, align 4
  %addtmp23 = add i32 %variableName22, 1
  store i32 %addtmp23, ptr %variableName, align 4
  %State12 = load i32, ptr %variableName, align 4
  ret i32 %State12
}
