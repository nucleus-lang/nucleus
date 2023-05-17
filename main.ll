; ModuleID = 'Nucleus'
source_filename = "Nucleus"

define i32 @main() {
entry:
  br i1 true, label %if, label %continue

if:                                               ; preds = %entry
  br label %continue

continue:                                         ; preds = %if, %entry
  %phi = phi i32 [ 15, %if ], [ 15, %entry ]
  %phi1 = phi i32 [ 6, %if ], [ 6, %entry ]
  %phi2 = phi i32 [ 10, %if ], [ 10, %entry ]
  %phi3 = phi i32 [ 5, %if ], [ 5, %entry ]
  %cmptmp = icmp eq i32 %phi, %phi3
  br i1 %cmptmp, label %if4, label %continue5

if4:                                              ; preds = %continue
  %addtmp = add i32 %phi2, 2
  br label %continue5

continue5:                                        ; preds = %if4, %continue
  %phi6 = phi i32 [ %phi, %if4 ], [ %phi, %continue ]
  %phi7 = phi i32 [ %phi1, %if4 ], [ %phi1, %continue ]
  %phi8 = phi i32 [ %addtmp, %if4 ], [ %addtmp, %continue ]
  %phi9 = phi i32 [ %phi3, %if4 ], [ %phi3, %continue ]
  ret i32 %phi8
}
