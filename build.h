#pragma once

#include <stddef.h>

struct project {
  char* _executable = NULL;
  char* _compiler = NULL;

  size_t _src_count = 0, _flag_count = 0, _include_dir_count = 0,
         _extra_files_count = 0;

  char** _src_files = NULL;
  char** _flags = NULL;
  char** _include_dirs = NULL;
  // only used by rebuild needed
  char** _extra_files = NULL;
};

extern struct project* current_project;
inline void set_current_project(struct project* p) { current_project = p; };
void free_project(struct project* p);

// Creates a new project setting the project's executable (incl path) and sets
// current_project
struct project* project(char const* executable);
void set_compiler_project(struct project* p, char const* compiler);
void add_src_dir_project(struct project* p, const char* dir);
void add_src_dir_recurse_project(struct project* p, const char* dir);
void add_include_dir_project(struct project* p, char const* dir);
void add_source_file_project(struct project* p, char const* file);
void add_flag_project(struct project* p, char const* flag);
int compile_project(struct project* p);
void add_extra_file_project(struct project* p, char const* file);
// Compares all source files (+ extra files) (not checking includes) and returns
// non 0 if modification time of any is newer than the executable's
int rebuild_needed_project(struct project* p);

// These functions operate on current_project
inline void set_compiler(char const* compiler) {
  if (current_project) set_compiler_project(current_project, compiler);
}

inline void add_src_dir(char const* dir) {
  if (current_project) add_src_dir_project(current_project, dir);
}

inline void add_src_dir_recurse(char const* dir) {
  if (current_project) add_src_dir_recurse_project(current_project, dir);
}

inline void add_include_dir(char const* dir) {
  if (current_project) add_include_dir_project(current_project, dir);
}

inline void add_source_file(char const* file) {
  if (current_project) add_source_file_project(current_project, file);
}

inline void add_flag(char const* flag) {
  if (current_project) add_flag_project(current_project, flag);
}

// if compile is successful return is not 0
inline int compile() {
  if (current_project)
    return compile_project(current_project);
  else
    return 0;
}

inline void add_extra_file(char const* file) {
  if (current_project) add_extra_file_project(current_project, file);
}

inline int rebuild_needed() {
  if (current_project)
    return rebuild_needed_project(current_project);
  else
    return 0;
}
