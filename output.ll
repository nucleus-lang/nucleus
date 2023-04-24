; ModuleID = 'Nucleus'
source_filename = "Nucleus"

define i32 @calculate_fib(i32 %n) {
entry:
  %subtmp = sub i32 %n, 1
  %subtmp1 = sub i32 %n, 2
  %cmptmp = icmp uge i32 1, %n
  br i1 %cmptmp, label %if, label %continue

if:                                               ; preds = %entry
  ret i32 %n
  br label %continue

continue:                                         ; preds = %if, %entry
  %calltmp = call i32 @calculate_fib(i32 %subtmp)
  %calltmp2 = call i32 @calculate_fib(i32 %subtmp1)
  %addtmp = add i32 %calltmp, %calltmp2
  ret i32 %addtmp
}

define i32 @main() {
entry:
  %calltmp = call i32 @calculate_fib(i32 15)
  ret i32 %calltmp
}
