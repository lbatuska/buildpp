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

struct project* current_project = NULL;
static inline void free_for_each_impl(char** data, size_t count) {
  for (size_t i = 0; i < count; i++) {
    free((void*)data[i]);
  }
};

void free_project(struct project* p) {
  free_for_each_impl(p->_extra_files, p->_extra_files_count);
  free(p->_extra_files);
  free_for_each_impl(p->_include_dirs, p->_include_dir_count);
  free(p->_include_dirs);
  free_for_each_impl(p->_src_files, p->_src_count);
  free(p->_src_files);
  free_for_each_impl(p->_flags, p->_flag_count);
  free(p->_flags);
  free((void*)p->_executable);
  free((void*)p->_compiler);
}

struct project* project(char const* executable) {
  struct project* p = (struct project*)malloc(sizeof(struct project));

  const size_t exec_len = strlen(executable);
  p->_executable = (char*)malloc(exec_len + 1);
  p->_executable[exec_len] = '\0';
  strncpy(p->_executable, executable, exec_len);

  p->_compiler = (char*)malloc(11);
  strcpy(p->_compiler, "musl-clang");

  return p;
}

void set_compiler_project(struct project* p, char const* compiler) {
  if (p->_compiler) free(p->_compiler);
  const size_t compiler_len = strlen(compiler);
  p->_compiler = (char*)malloc(compiler_len + 1);
  p->_compiler[compiler_len] = '\0';
  strncpy(p->_compiler, compiler, compiler_len);
}

void add_src_dir_project(struct project* p, const char* dir) {
  struct dirent* entry;
  DIR* directory = opendir(dir);
  if (!directory) {
    fprintf(stderr, "opendir (%s) failed\n", dir);
    return;
  }

  // Add a trailing slash
  const size_t orig_len = strlen(dir);
  bool alloced = false;
  if (dir[orig_len - 1] != '/') {
    char* d = (char*)malloc(sizeof(char) * (orig_len + 2));
    strncpy(d, dir, orig_len);
    d[orig_len] = '/';
    d[orig_len + 1] = '\0';
    // we allocated dir, we'll need to free it
    alloced = true;
    dir = d;
  }

  const size_t dirlen = alloced ? orig_len + 1 : orig_len;

  while ((entry = readdir(directory)) != NULL) {
    if (entry->d_type == DT_REG) {  // Regular file
      const char* ext = strrchr(entry->d_name, '.');
      if (ext && (strcmp(ext, ".c") == 0 || strcmp(ext, ".cpp") == 0)) {
        const size_t filelen = strlen(entry->d_name);
        // make malloced path
        char* fullpath = (char*)malloc(dirlen + filelen + 1);
        fullpath[dirlen + filelen] = '\0';
        // dir path + filename
        strncpy(fullpath, dir, dirlen);
        strncpy(fullpath + dirlen, entry->d_name, filelen);
        // add to source file
        p->_src_files =
            (char**)realloc(p->_src_files, sizeof(char*) * (p->_src_count + 1));
        p->_src_files[p->_src_count++] = fullpath;
      }
    }
  }

  closedir(directory);

  if (alloced) {
    free((void*)dir);
  }
}

void add_src_dir_recurse_project(struct project* p, const char* dirpath) {
  struct dirent* entry;
  DIR* dir = opendir(dirpath);

  if (!dir) {
    fprintf(stderr, "opendir recurse (%s) failed\n", dirpath);
    return;
  }

  add_src_dir_project(p, dirpath);

  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    char fullpath[1024];
    snprintf(fullpath, sizeof(fullpath), "%s/%s", dirpath, entry->d_name);

    struct stat path_stat;
    if (stat(fullpath, &path_stat) == 0 && S_ISDIR(path_stat.st_mode)) {
      add_src_dir_recurse_project(p, fullpath);  // Recurse into subdirectory
    }
  }

  closedir(dir);
}

void add_include_dir_project(struct project* p, char const* dir) {
  p->_include_dirs = (char**)realloc(
      p->_include_dirs, sizeof(char*) * (p->_include_dir_count + 1));
  p->_include_dirs[p->_include_dir_count++] = strdup(dir);
}

void add_source_file_project(struct project* p, char const* file) {
  p->_src_files =
      (char**)realloc(p->_src_files, sizeof(char*) * (p->_src_count + 1));
  p->_src_files[p->_src_count++] = strdup(file);
}

void add_flag_project(struct project* p, char const* flag) {
  p->_flags = (char**)realloc(p->_flags, sizeof(char*) * (p->_flag_count + 1));
  p->_flags[p->_flag_count++] = strdup(flag);
}

int compile_project(struct project* p) {
  // 1+ compiler path +2 for "-o" "executable"  +1 for NULL
  const size_t argv_len = (1 + p->_src_count + p->_flag_count +
                           (p->_include_dir_count * 2) + 2 + 1);
  char* argv[argv_len];
  size_t argc = 0;
  argv[argc++] = p->_compiler;

  char* dash_i = strdup("-I");
  for (int i = 0; i < p->_include_dir_count; i++) {
    argv[argc++] = dash_i;
    argv[argc++] = p->_include_dirs[i];
  }

  for (int i = 0; i < p->_flag_count; i++) {
    argv[argc++] = p->_flags[i];
  }

  for (int i = 0; i < p->_src_count; i++) {
    argv[argc++] = p->_src_files[i];
  }

  // End
  argv[argc++] = strdup("-o");    // argv_len-3
  argv[argc++] = p->_executable;  // argv_len-2
  argv[argc++] = NULL;            // argv_len-1

  printf("=> ");
  for (int i = 0; i < argc; i++) {
    printf("%s ", argv[i]);
  }
  printf("\n");

  if (fork() == 0) {
    execvp(p->_compiler, argv);
    perror("execvp failed\n");
    exit(1);
  }
  int status;
  wait(&status);
  if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
    return 1;
  }
  perror("exited with status != 0\n");
  exit(1);

  return 0;
}

void add_extra_file_project(struct project* p, char const* file) {
  p->_extra_files_count++;
  p->_extra_files =
      (char**)realloc(p->_extra_files, sizeof(char*) * (p->_extra_files_count));
  const size_t file_len = strlen(file);
  p->_extra_files[p->_extra_files_count - 1] = (char*)malloc(file_len + 1);
  p->_extra_files[p->_extra_files_count - 1][file_len] = '\0';
  strncpy(p->_extra_files[p->_extra_files_count - 1], file, file_len);
}

static inline bool newer_file(long int exec_st_mtime, char** files,
                              size_t files_len) {
  struct stat file_stat;

  for (size_t i = 0; i < files_len; i++) {
    if (stat(files[i], &file_stat) == 0) {
      if (file_stat.st_mtime > exec_st_mtime) {
        return true;
      }
    }
  }
  return false;
}

int rebuild_needed_project(struct project* p) {
  struct stat exec_stat;
  if (stat(p->_executable, &exec_stat) != 0) return 1;
  if (newer_file(exec_stat.st_mtime, p->_src_files, p->_src_count)) return 1;
  if (newer_file(exec_stat.st_mtime, p->_extra_files, p->_extra_files_count))
    return 1;

  return 0;
}
