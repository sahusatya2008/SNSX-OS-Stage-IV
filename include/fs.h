#ifndef FS_H
#define FS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FS_PATH_MAX 64
#define FS_MAX_ENTRY_SIZE 16384

typedef struct fs_entry {
    const char *name;
    const uint8_t *data;
    size_t size;
    uint32_t type;
} fs_entry_t;

enum {
    FS_TYPE_TEXT = 1,
    FS_TYPE_PROGRAM = 2,
    FS_TYPE_DIR = 3
};

bool fs_init(uint32_t multiboot_magic, uintptr_t multiboot_info_addr);
size_t fs_count(void);
const fs_entry_t *fs_get(size_t index);
const fs_entry_t *fs_find(const char *path);
size_t fs_list(const char *directory, const fs_entry_t **results, size_t max_results);
bool fs_mkdir(const char *path);
bool fs_write_file(const char *path, const uint8_t *data, size_t size, uint32_t type);
bool fs_append_file(const char *path, const uint8_t *data, size_t size);
bool fs_remove(const char *path);
bool fs_copy(const char *source, const char *destination);
bool fs_move(const char *source, const char *destination);
bool fs_exists(const char *path);
bool fs_is_dir(const char *path);

#endif
