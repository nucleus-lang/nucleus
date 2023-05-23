; ModuleID = 'Nucleus'
source_filename = "Nucleus"

; Function Attrs: mustprogress nofree nounwind willreturn
define i32 @calculate_fib(i32 %n) #0 {
entry:
  %cmptmp = icmp uge i32 1, %n
  br i1 %cmptmp, label %if, label %continue

if:                                               ; preds = %entry
  ret i32 %n
  br label %continue

continue:                                         ; preds = %if, %entry
  %subtmp = sub i32 %n, 1
  %subtmp1 = sub i32 %n, 2
  %calltmp = tail call i32 @calculate_fib(i32 %subtmp)
  %calltmp2 = tail call i32 @calculate_fib(i32 %subtmp1)
  %addtmp = add i32 %calltmp, %calltmp2
  ret i32 %addtmp
}

; Function Attrs: mustprogress nofree nounwind willreturn
define i32 @main() #0 {
entry:
  %calltmp = tail call i32 @calculate_fib(i32 30)
  ret i32 %calltmp
}

attributes #0 = { mustprogress nofree nounwind willreturn }
