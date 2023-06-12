; ModuleID = 'Nucleus'
source_filename = "Nucleus"

; Function Attrs: mustprogress nofree nounwind willreturn
define i32 @main() #0 {
entry:
  %my_first_array = alloca [4 x i32], align 4
  %my_first_array1 = load [4 x i32], ptr %my_first_array, align 4
  br i1 true, label %if, label %continue

if:                                               ; preds = %entry
  ret i32 1

continue:                                         ; preds = %entry
  %first_element = getelementptr [4 x i32], ptr %my_first_array, i32 0, i32 5
  %first_element2 = load i32, ptr %first_element, align 4
  %addtmp = add i32 %first_element2, 4
  %addtmp3 = add i32 %addtmp, 8
  %addtmp4 = add i32 %addtmp3, 16
  store i32 %addtmp4, ptr %first_element, align 4
  %autoLoad = load i32, ptr %first_element, align 4
  ret i32 %autoLoad
}

attributes #0 = { mustprogress nofree nounwind willreturn }
