; ModuleID = 'Nucleus'
source_filename = "Nucleus"

define i32 @main() {
entry:
  br i1 false, label %if, label %else

if:                                               ; preds = %entry
  br label %continue

else:                                             ; preds = %entry
  br label %continue

continue:                                         ; preds = %else, %if
  %phi = phi i32 [ 2, %if ], [ 2, %else ]
  %phi1 = phi i32 [ 6, %if ], [ 6, %else ]
  %phi2 = phi i32 [ 10, %if ], [ 2, %else ]
  %addtmp = add i32 %phi2, 3
  ret i32 %addtmp
}
