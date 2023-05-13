; ModuleID = 'main.ll'
source_filename = "Nucleus"
target datalayout = "e-m:w-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-w64-windows-gnu"

; Function Attrs: mustprogress nofree norecurse nosync nounwind readnone willreturn
define i64 @argument_test(i64 %b1, i64 %b2, i64 %b3, i64 %b4, i64 %b5, i64 %b6, i64 %b7, i64 %b8, i64 %b9) local_unnamed_addr #0 {
entry:
  %addtmp = add i64 %b2, %b1
  %addtmp1 = add i64 %addtmp, %b3
  %addtmp2 = add i64 %addtmp1, %b4
  %addtmp3 = add i64 %addtmp2, %b5
  %addtmp4 = add i64 %addtmp3, %b6
  %addtmp5 = add i64 %addtmp4, %b7
  %addtmp6 = add i64 %addtmp5, %b8
  %addtmp7 = add i64 %addtmp6, %b9
  ret i64 %addtmp7
}

attributes #0 = { mustprogress nofree norecurse nosync nounwind readnone willreturn uwtable[async] }
