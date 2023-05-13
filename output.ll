; ModuleID = 'Nucleus'
source_filename = "Nucleus"

define i64 @calculate_fib(i64 %n) {
entry:
  %cmptmp = icmp uge i64 1, %n
  br i1 %cmptmp, label %if, label %continue

if:                                               ; preds = %entry
  ret i64 %n
  br label %continue

continue:                                         ; preds = %if, %entry
  %subtmp = sub i64 %n, 1
  %calltmp = call i64 @calculate_fib(i64 %subtmp)
  %subtmp1 = sub i64 %n, 2
  %calltmp2 = call i64 @calculate_fib(i64 %subtmp1)
  %addtmp = add i64 %calltmp, %calltmp2
  ret i64 %addtmp
}

define i64 @main() {
entry:
  ret i64 45
}
