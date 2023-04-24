; ModuleID = 'main.ll'
source_filename = "Nucleus"
target datalayout = "e-m:w-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-w64-windows-gnu"

; Function Attrs: nofree nosync nounwind readnone
define i32 @calculate_fib(i32 %n) local_unnamed_addr #0 {
entry:
  %cmptmp1 = icmp ult i32 %n, 2
  br i1 %cmptmp1, label %common.ret, label %continue

common.ret:                                       ; preds = %continue, %entry
  %accumulator.tr.lcssa = phi i32 [ 0, %entry ], [ %addtmp, %continue ]
  %n.tr.lcssa = phi i32 [ %n, %entry ], [ %subtmp1, %continue ]
  %accumulator.ret.tr = add i32 %n.tr.lcssa, %accumulator.tr.lcssa
  ret i32 %accumulator.ret.tr

continue:                                         ; preds = %entry, %continue
  %n.tr3 = phi i32 [ %subtmp1, %continue ], [ %n, %entry ]
  %accumulator.tr2 = phi i32 [ %addtmp, %continue ], [ 0, %entry ]
  %subtmp = add i32 %n.tr3, -1
  %calltmp = tail call i32 @calculate_fib(i32 %subtmp)
  %subtmp1 = add i32 %n.tr3, -2
  %addtmp = add i32 %calltmp, %accumulator.tr2
  %cmptmp = icmp ult i32 %subtmp1, 2
  br i1 %cmptmp, label %common.ret, label %continue
}

; Function Attrs: nofree nosync nounwind readnone
define i32 @main() local_unnamed_addr #0 {
entry:
  %calltmp = tail call i32 @calculate_fib(i32 30)
  ret i32 %calltmp
}

attributes #0 = { nofree nosync nounwind readnone }
