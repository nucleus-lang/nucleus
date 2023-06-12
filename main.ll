; ModuleID = 'Nucleus'
source_filename = "Nucleus"

; Function Attrs: mustprogress nofree nounwind willreturn
define i32 @main() #0 {
entry:
  %my_first_array = alloca [4 x i32], align 4
  %my_first_array1 = load [4 x i32], ptr %my_first_array, align 4
  %getelement = getelementptr [4 x i32], ptr %my_first_array, i32 0, i32 0
  store volatile i32 1, ptr %getelement, align 4
  %getelement2 = getelementptr [4 x i32], ptr %my_first_array, i32 0, i32 1
  store volatile i32 2, ptr %getelement2, align 4
  %getelement3 = getelementptr [4 x i32], ptr %my_first_array, i32 0, i32 2
  store volatile i32 3, ptr %getelement3, align 4
  %my_first_array4 = load [4 x i32], ptr %my_first_array, align 4
  br i1 false, label %if, label %continue

if:                                               ; preds = %entry
  ret i32 1

continue:                                         ; preds = %entry
  %first_element = getelementptr [4 x i32], ptr %my_first_array, i32 0, i32 2
  %first_element5 = load i32, ptr %first_element, align 4
  %addtmp = add i32 %first_element5, 4
  store i32 %addtmp, ptr %first_element, align 4
  %autoLoad = load i32, ptr %first_element, align 4
  ret i32 %autoLoad
}

attributes #0 = { mustprogress nofree nounwind willreturn }
