; ModuleID = 'Nucleus'
source_filename = "Nucleus"

; Function Attrs: mustprogress nofree nounwind willreturn
define ghccc i32 @main() #0 {
entry:
  br i1 false, label %if, label %continue

if:                                               ; preds = %entry
  br label %continue

continue:                                         ; preds = %if, %entry
  br i1 true, label %if1, label %continue4

if1:                                              ; preds = %continue
  br i1 true, label %if2, label %continue3

if2:                                              ; preds = %if1
  br label %continue3

continue3:                                        ; preds = %if2, %if1
  %phi = phi i32 [ 5, %if2 ], [ 0, %if1 ]
  %addtmp = add i32 %phi, 3
  br label %continue4

continue4:                                        ; preds = %continue3, %continue
  %phi5 = phi i32 [ %addtmp, %continue3 ], [ 0, %continue ]
  ret i32 %phi5
}

attributes #0 = { mustprogress nofree nounwind willreturn }
