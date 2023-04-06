; ModuleID = 'Nucleus'
source_filename = "Nucleus"

define i32 @main() {
entry:
  %one = alloca i32, align 4
  store i32 0, ptr %one, align 4
  %one1 = load i32, ptr %one, align 4
  %sexttmp = sext i32 %one1 to i64
  %addtmp = add i64 %sexttmp, 4000000000
  %trunctmp = trunc i64 %addtmp to i32
  store i32 %trunctmp, ptr %one, align 4
  %autoLoad = load i32, ptr %one, align 4
  ret i32 %autoLoad
}
