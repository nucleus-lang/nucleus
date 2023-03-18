; ModuleID = 'Nucleus'
source_filename = "Nucleus"

define i32 @main() {
entry:
  %boolean = alloca i32, align 4
  store i32 1, ptr %boolean, align 4
  %autoLoad = load i32, ptr %boolean, align 4
  ret i32 %autoLoad
}
