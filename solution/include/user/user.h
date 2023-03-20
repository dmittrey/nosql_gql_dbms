#pragma once

#include "utils/utils.h"

#include "json/iter.h"
#include "json/object/type.h"

#include "file/file.h"

struct File *user_open_file(const char *const name);
Status user_close_file(struct File *const file);

Status user_read_type(struct File *const file, const String *const name, Type *const o_type);
Status user_add_type(struct File *const file, const Type *const type);
Status user_delete_type(struct File *const file, const String *const name);

Status user_write(struct File *const file, const Json *const json, const String *const type_name);
struct Iter *user_read(struct File *const file, Query *const query);
Status user_update(struct File *const file, Query *const query, const Json *const new_json);
Status user_delete(struct File *const file, Query *const query);