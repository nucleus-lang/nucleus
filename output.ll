; ModuleID = 'Nucleus'
source_filename = "Nucleus"

define i32 @check_if_bigger_than(i32 %a, i32 %b) {
entry:
  %cmptmp = icmp ugt i32 %a, %b
  br i1 %cmptmp, label %if, label %continue

if:                                               ; preds = %entry
  br label %continue

continue:                                         ; preds = %if, %entry
  %phi = phi i32 [ 10, %if ], [ 0, %entry ]
  %cmptmp1 = icmp ugt i32 %a, 30
  br i1 %cmptmp1, label %if2, label %continue3

if2:                                              ; preds = %continue
  %addtmp = add i32 %phi, 5
  br label %continue3

continue3:                                        ; preds = %if2, %continue
  %phi4 = phi i32 [ %addtmp, %if2 ], [ %phi, %continue ]
  ret i32 %phi4
}

define i32 @main() {
entry:
  %calltmp = call i32 @check_if_bigger_than(i32 15, i32 6)
  ret i32 %calltmp
}
