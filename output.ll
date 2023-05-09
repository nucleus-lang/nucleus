; ModuleID = 'Nucleus'
source_filename = "Nucleus"

define i32 @main() {
entry:
  br i1 true, label %while, label %continue

while:                                            ; preds = %while, %entry
  %phi = phi i32 [ %addtmp, %while ], [ 0, %entry ]
  %phi1 = phi i32 [ %addtmp2, %while ], [ 0, %entry ]
  %addtmp = add i32 %phi, 1
  %addtmp2 = add i32 %phi1, 10
  %cmptmp = icmp ugt i32 100, %addtmp
  br i1 %cmptmp, label %while, label %continue

continue:                                         ; preds = %while, %entry
  %phi3 = phi i32 [ 0, %entry ], [ %addtmp2, %while ]
  ret i32 %phi3
}
