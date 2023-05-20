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
  br i1 false, label %if2, label %else

if2:                                              ; preds = %if1
  %addtmp = add i32 %phi, 5
  br label %continue4

else:                                             ; preds = %if1
  %addtmp3 = add i32 %phi, 3
  br label %continue4

continue4:                                        ; preds = %else, %if2
  %phi5 = phi i32 [ %addtmp, %if2 ], [ %addtmp3, %else ]
  br label %continue6

continue6:                                        ; preds = %continue4, %continue
  %phi7 = phi i32 [ %phi5, %if1 ], [ %phi, %continue ]
  ret i32 %phi7
}
