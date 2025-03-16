#pragma once

void set_compiler(char const* compiler);
void set_compiler_self(char const* compiler);
void rebuild_self(char const* build, char const* self_source,
                  char const* self_exec, bool Static);
void executable(char const* executable);
void add_src_dir(char const* dir);
void add_src_dir_recurse(char const* dir);
void add_include_dir(char const* dir);
void add_source_file(char const* file);
void add_flag(char const* flag);
void compile();
