; ModuleID = 'Nucleus'
source_filename = "Nucleus"

; Function Attrs: mustprogress nofree nounwind willreturn
define i32 @main() #0 {
entry:
  %my_first_array = alloca [4 x i32], align 4
  %my_first_array1 = load [4 x i32], ptr %my_first_array, align 4
  %first_element = getelementptr [4 x i32], ptr %my_first_array, i32 0, i32 3
  %first_element2 = load i32, ptr %first_element, align 4
  %addtmp = add i32 %first_element2, 4
  store i32 %addtmp, ptr %first_element, align 4
  %my_first_array3 = load [4 x i32], ptr %my_first_array, align 4
  %getelement = getelementptr [4 x i32], ptr %my_first_array, i32 0, i32 3
  %getelement4 = load i32, ptr %getelement, align 4
  ret i32 %getelement4
}

attributes #0 = { mustprogress nofree nounwind willreturn }
