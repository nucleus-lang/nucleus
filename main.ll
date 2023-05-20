; ModuleID = 'Nucleus'
source_filename = "Nucleus"

define i32 @main() {
entry:
  br i1 true, label %if, label %continue

if:                                               ; preds = %entry
  br label %continue

continue:                                         ; preds = %if, %entry
  %phi = phi i32 [ 10, %if ], [ 0, %entry ]
  br i1 false, label %if1, label %continue6

if1:                                              ; preds = %continue
  br i1 false, label %if2, label %continue3

if2:                                              ; preds = %if1
  %addtmp = add i32 %phi, 5
  br label %continue3

continue3:                                        ; preds = %if2, %if1
  %phi4 = phi i32 [ %addtmp, %if2 ], [ %phi, %if1 ]
  %addtmp5 = add i32 %phi4, 3
  br label %continue6

continue6:                                        ; preds = %continue3, %continue
  %phi7 = phi i32 [ %addtmp5, %continue3 ], [ %phi, %continue ]
  ret i32 %phi7
}
