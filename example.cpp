#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "build.h"

int main(int argc, char* argv[]) {
  // === SELF ===
  struct project* self = project(argv[0]);
  add_source_file_project(self, "./buildpp/build.cpp");
  add_source_file_project(self, "./build.cpp");
  set_compiler_project(self, "musl-clang");
  add_extra_file_project(self, "./buildpp/build.h");
  add_flag_project(self, "-static");
  setbuf(stdout, NULL);
  if (rebuild_needed_project(self)) {
    printf("Rebuild needed!\n");
    if (compile_project(self)) {
      free_project(self);
      execl(argv[0], argv[0], NULL);
      perror("execl failed");
      exit(1);
    } else {
      perror("Rebuild failed!\n");
      exit(1);
    }
  }
  // === SELF ===

  // add an actual project here
  return 0;
}
