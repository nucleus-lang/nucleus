; ModuleID = 'Nucleus'
source_filename = "Nucleus"

define i8 @main() {
entry:
  %one = alloca i8, align 1
  store i8 0, ptr %one, align 1
  %two = load i8, ptr %one, align 1
  %sexttmp = sext i8 %two to i16
  %addtmp = add i16 %sexttmp, 255
  %trunctmp = trunc i16 %addtmp to i8
  %one1 = load i8, ptr %one, align 1
  %addtmp2 = add i8 %one1, %trunctmp
  %addtmp3 = add i8 %addtmp2, 1
  %addtmp4 = add i8 %addtmp3, 2
  %addtmp5 = add i8 %addtmp4, 3
  store i8 %addtmp5, ptr %one, align 1
  %autoLoad = load i8, ptr %one, align 1
  ret i8 %autoLoad
}
