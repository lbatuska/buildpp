#include "build.h"

int main(int argc, char *argv[]) {
  set_compiler_self("musl-clang");
  rebuild_self("./buildpp/build", "./buildpp/example.cpp", argv[0], true);
  set_compiler("clang");
  executable("./program");
  add_src_dir_recurse("./src");
  add_include_dir("./include");
  add_source_file("./main.cpp");
  add_flag("-Wall");
  // add_flag("-static");
  add_flag("-lstdc++");
  compile();
  return 0;
}
