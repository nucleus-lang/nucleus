; ModuleID = 'Nucleus'
source_filename = "Nucleus"

define i32 @calculate_fib(i32 %n) {
entry:
  %cmptmp = icmp uge i32 1, %n
  br i1 %cmptmp, label %if, label %continue

if:                                               ; preds = %entry
  ret i32 %n
  br label %continue

continue:                                         ; preds = %if, %entry
  %subtmp = sub i32 %n, 1
  %calltmp = call i32 @calculate_fib(i32 %subtmp)
  %subtmp1 = sub i32 %n, 2
  %calltmp2 = call i32 @calculate_fib(i32 %subtmp1)
  %addtmp = add i32 %calltmp, %calltmp2
  %addtmp3 = add i32 %addtmp, %calltmp2
  ret i32 %addtmp
}

define i32 @generate_zero() {
entry:
  ret i32 0
}

define i32 @main() {
entry:
  %calltmp = call i32 @generate_zero()
  %calltmp1 = call i32 @generate_zero()
  %cmptmp = icmp ugt i32 100, %calltmp
  br i1 %cmptmp, label %while, label %continue

while:                                            ; preds = %while, %entry
  %phi = phi i32 [ %addtmp, %while ], [ %calltmp, %entry ]
  %phi2 = phi i32 [ %addtmp3, %while ], [ %calltmp1, %entry ]
  %addtmp = add i32 %phi, 1
  %addtmp3 = add i32 %phi2, 10
  %cmptmp4 = icmp ugt i32 100, %addtmp
  br i1 %cmptmp4, label %while, label %continue

continue:                                         ; preds = %while, %entry
  %phi5 = phi i32 [ %calltmp1, %entry ], [ %addtmp3, %while ]
  ret i32 %phi5
}
