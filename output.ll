; ModuleID = 'Nucleus'
source_filename = "Nucleus"

define i32 @main() {
entry:
  %core = alloca i32, align 4
  store i32 0, ptr %core, align 4
  %snake_x = load i32, ptr %core, align 4
  %snake_y = load i32, ptr %core, align 4
  %cmptmp = icmp ugt i32 100, %snake_y
  br i1 %cmptmp, label %while, label %continue

while:                                            ; preds = %while, %entry
  %phi = phi i32 [ %addtmp, %while ], [ %snake_x, %entry ]
  %phi1 = phi i32 [ %addtmp2, %while ], [ %snake_y, %entry ]
  %addtmp = add i32 %phi, 100
  %addtmp2 = add i32 %phi1, 1
  %cmptmp3 = icmp ugt i32 100, %addtmp2
  br i1 %cmptmp3, label %while, label %continue

continue:                                         ; preds = %while, %entry
  %phi4 = phi i32 [ %snake_x, %entry ], [ %addtmp, %while ]
  %addtmp5 = add i32 %phi4, 0
  ret i32 %addtmp5
}
