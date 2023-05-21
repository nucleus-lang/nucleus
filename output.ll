; ModuleID = 'Nucleus'
source_filename = "Nucleus"

; Function Attrs: mustprogress nofree nounwind willreturn
define i32 @main() #0 {
entry:
  %l = alloca i32, align 4
  store i32 0, ptr %l, align 4
  %l1 = load i32, ptr %l, align 4
  br i1 true, label %if, label %continue

if:                                               ; preds = %entry
  br label %continue

continue:                                         ; preds = %if, %entry
  %phi = phi i32 [ 10, %if ], [ 0, %entry ]
  br i1 true, label %if2, label %continue7

if2:                                              ; preds = %continue
  br i1 false, label %if3, label %continue4

if3:                                              ; preds = %if2
  %addtmp = add i32 %phi, 5
  br label %continue4

continue4:                                        ; preds = %if3, %if2
  %phi5 = phi i32 [ %addtmp, %if3 ], [ %phi, %if2 ]
  %addtmp6 = add i32 %phi5, 3
  br label %continue7

continue7:                                        ; preds = %continue4, %continue
  %phi8 = phi i32 [ %addtmp6, %continue4 ], [ %phi, %continue ]
  %addtmp9 = add i32 %l1, %phi8
  store i32 %addtmp9, ptr %l, align 4
  %autoLoad = load i32, ptr %l, align 4
  ret i32 %autoLoad
}

attributes #0 = { mustprogress nofree nounwind willreturn }
