#include "build.h"

#include <dirent.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

const char* _compiler = "musl-clang";
const char* _compiler_self = "musl-clang";
const char* _output_exe = "./program";

static size_t _src_count = 0, _flag_count = 0, _include_dir_count = 0;

char const** _flags = NULL;
char const** _src_files = NULL;
char const** _include_dirs = NULL;

void set_compiler(char const* compiler) { _compiler = compiler; }
void set_compiler_self(char const* compiler) { _compiler_self = compiler; }
void executable(char const* executable) { _output_exe = executable; }

/*
@param build the path to the build.{h|cpp} file without extension, eg ./build
@param self_source path with extension to the builder's cpp file
@param self_exec we can pass argv(0) to this directly
@param Static appends -static flag to the self compile command

*/
void rebuild_self(char const* build, char const* self_source,
                  char const* self_exec, bool Static) {
  struct stat build_cpp, build_h, selfs, selfe;
  size_t len = strlen(build);

  char* h_path = (char*)malloc(sizeof(char) * (len + 1 + 2));
  char* cpp_path = (char*)malloc(sizeof(char) * (len + 1 + 4));

  memcpy(h_path, build, len);
  memcpy(cpp_path, build, len);
  strncpy(h_path + len, ".h\0", 3);
  strncpy(cpp_path + len, ".cpp\0", 5);

  if (stat(cpp_path, &build_cpp) != 0 || stat(h_path, &build_h) != 0 ||
      stat(self_source, &selfs) != 0 || stat(self_exec, &selfe) != 0) {
    perror("stat failed\n");
    exit(1);
  }
  setbuf(stdout, NULL);
  if (build_cpp.st_mtime > selfe.st_mtime ||
      build_h.st_mtime > selfe.st_mtime || selfs.st_mtime > selfe.st_mtime) {
    printf("Rebuild needed! -> ");
    if (fork() == 0) {
      if (Static) {
        printf("(%s %s %s -static -o %s) -> ", _compiler, self_source, cpp_path,
               self_exec);
        execlp(_compiler, _compiler, self_source, cpp_path, "-static", "-o",
               self_exec, NULL);
      } else {
        printf("(%s %s %s -o %s) -> ", _compiler, self_source, cpp_path,
               self_exec);
        execlp(_compiler, _compiler, self_source, cpp_path, "-o", self_exec,
               NULL);
      }
      perror("execlp failed\n");
      exit(1);
    }
    int status;
    wait(&status);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
      printf("Rebuild success! Calling self.\n");
      execl(self_exec, self_exec, NULL);
      perror("execl failed");
      exit(1);
    }
    perror("exited with status != 0\n");
    exit(1);
  }
  printf("No rebuild needed!\n");
  return;
}
void add_src_dir(char const* dir) {
  struct dirent* entry;
  DIR* directory = opendir(dir);
  if (!directory) {
    fprintf(stderr, "opendir (%s) failed\n", dir);
    return;
  }

  const size_t orig_len = strlen(dir);
  bool alloced = false;
  if (dir[orig_len - 1] != '/') {
    char* d = (char*)malloc(sizeof(char) * (orig_len + 2));
    strncpy(d, dir, orig_len);
    d[orig_len] = '/';
    d[orig_len + 1] = '\0';
    alloced = true;
    dir = d;
  }

  const size_t dirlen = strlen(dir);

  while ((entry = readdir(directory)) != NULL) {
    if (entry->d_type == DT_REG) {  // Regular file
      const char* ext = strrchr(entry->d_name, '.');
      if (ext && (strcmp(ext, ".c") == 0 || strcmp(ext, ".cpp") == 0)) {
        const size_t filelen = strlen(entry->d_name);
        char* fullpath = (char*)malloc(dirlen + filelen + 1);
        fullpath[dirlen + filelen] = '\0';
        strncpy(fullpath, dir, dirlen);
        strncpy(fullpath + dirlen, entry->d_name, filelen);
        add_source_file(fullpath);
      }
    }
  }

  closedir(directory);
  if (alloced) {
    free((void*)dir);
  }
}

void add_src_dir_recurse(char const* dirpath) {
  struct dirent* entry;
  DIR* dir = opendir(dirpath);

  if (!dir) {
    fprintf(stderr, "opendir recurse (%s) failed\n", dirpath);
    return;
  }

  add_src_dir(dirpath);  // Call list_c_cpp_files for current directory

  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    char fullpath[1024];
    snprintf(fullpath, sizeof(fullpath), "%s/%s", dirpath, entry->d_name);

    struct stat path_stat;
    if (stat(fullpath, &path_stat) == 0 && S_ISDIR(path_stat.st_mode)) {
      add_src_dir_recurse(fullpath);  // Recurse into subdirectory
    }
  }

  closedir(dir);
}

void add_source_file(char const* file) {
  printf("Adding src file %s\n", file);
  _src_count++;
  _src_files = (char const**)realloc(_src_files, sizeof(char*) * _src_count);
  _src_files[_src_count - 1] = file;
}

void add_flag(char const* flag) {
  _flag_count++;
  _flags = (char const**)realloc(_flags, sizeof(char*) * _flag_count);
  _flags[_flag_count - 1] = flag;
}

void add_include_dir(char const* dir) {
  _include_dir_count++;
  _include_dirs =
      (char const**)realloc(_include_dirs, sizeof(char*) * _include_dir_count);
  _include_dirs[_include_dir_count - 1] = dir;
}

void compile() {
  // 1+ compiler
  // +2 for "-o" "executable"
  // +1 for NULL
  const size_t len =
      1 + _flag_count + _src_count + (_include_dir_count * 2) + 2 + 1;
  char* argv[len];
  int argc = 0;

  argv[0] = (char*)_compiler;
  argc++;
  for (int i = 0; i < _include_dir_count; i++) {
    argv[argc] = (char*)"-I";
    argc++;

    argv[argc] = (char*)_include_dirs[i];
    argc++;
  }

  for (int i = 0; i < _flag_count; i++) {
    argv[argc] = (char*)_flags[i];
    argc++;
  }

  for (int i = 0; i < _src_count; i++) {
    argv[argc] = (char*)_src_files[i];
    argc++;
  }

  argv[len - 3] = (char*)"-o";
  argc++;
  argv[len - 2] = (char*)_output_exe;
  argc++;

  argv[len - 1] = NULL;
  setbuf(stdout, NULL);
  printf("Will run:\n");
  for (int i = 0; i < argc; i++) {
    printf("%s ", argv[i]);
  }
  printf("\n");

  if (fork() == 0) {
    execvp(_compiler, argv);
    perror("execvp failed\n");
    exit(1);
  }
  int status;
  wait(&status);
  if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
    printf("Compilation success!\n");
    exit(0);
  }
  perror("exited with status != 0\n");
  exit(1);
}
