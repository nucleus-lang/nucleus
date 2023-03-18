; ModuleID = 'Nucleus'
source_filename = "Nucleus"

define i1 @main() {
entry:
  %boolean = alloca i32, align 4
  store i1 true, ptr %boolean, align 1
  %autoLoad = load i32, ptr %boolean, align 4
  ret i32 %autoLoad
}
