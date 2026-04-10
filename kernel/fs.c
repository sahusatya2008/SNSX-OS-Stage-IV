#include "common.h"
#include "fs.h"
#include "multiboot.h"
#include "string.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define INITRD_MAGIC 0x4D594653u
#define INITRD_VERSION 1u
#define INITRD_NAME_MAX 32
#define MAX_FS_ENTRIES 64

typedef struct initrd_header {
    uint32_t magic;
    uint32_t version;
    uint32_t file_count;
} PACKED initrd_header_t;

typedef struct initrd_node {
    char name[INITRD_NAME_MAX];
    uint32_t offset;
    uint32_t size;
    uint32_t type;
} PACKED initrd_node_t;

typedef struct fs_node {
    char name[FS_PATH_MAX];
    uint8_t data[FS_MAX_ENTRY_SIZE];
    size_t size;
    uint32_t type;
    fs_entry_t entry;
} fs_node_t;

static fs_node_t g_nodes[MAX_FS_ENTRIES];
static size_t g_entry_count;

static const char builtin_readme[] =
    "SNSX fallback filesystem\n"
    "This path is used when the custom boot image starts without an initrd.\n"
    "The live ISO includes the richer rootfs module.\n";

static const char builtin_home[] =
    "Welcome to SNSX.\n"
    "Use help, apps, files, calc, and edit to explore the live system.\n";

static const char builtin_motd[] =
    "SNSX live system: writable RAM filesystem, file manager, calculator, and editor.\n";

static void fs_sync_entry(fs_node_t *node) {
    node->entry.name = node->name;
    node->entry.data = node->data;
    node->entry.size = node->size;
    node->entry.type = node->type;
}

static bool fs_normalize_path(const char *input, char *output) {
    size_t in = 0;
    size_t out = 0;
    bool last_was_slash = false;

    if (input == NULL || input[0] == '\0') {
        output[0] = '\0';
        return true;
    }

    while (input[in] != '\0') {
        char ch = input[in++];
        if (ch == '/') {
            if (out == 0 || last_was_slash) {
                last_was_slash = true;
                continue;
            }
            if (out + 1 >= FS_PATH_MAX) {
                return false;
            }
            output[out++] = '/';
            last_was_slash = true;
            continue;
        }
        if (out + 1 >= FS_PATH_MAX) {
            return false;
        }
        output[out++] = ch;
        last_was_slash = false;
    }

    while (out > 0 && output[out - 1] == '/') {
        --out;
    }
    output[out] = '\0';
    return true;
}

static bool fs_parent_path(const char *normalized, char *parent) {
    const char *slash = strrchr(normalized, '/');
    if (slash == NULL) {
        parent[0] = '\0';
        return true;
    }

    size_t length = (size_t)(slash - normalized);
    if (length >= FS_PATH_MAX) {
        return false;
    }

    memcpy(parent, normalized, length);
    parent[length] = '\0';
    return true;
}

static bool fs_is_direct_child(const char *directory, const char *candidate) {
    if (directory[0] == '\0') {
        return strchr(candidate, '/') == NULL;
    }

    size_t dir_len = strlen(directory);
    if (strncmp(candidate, directory, dir_len) != 0 || candidate[dir_len] != '/') {
        return false;
    }
    return strchr(candidate + dir_len + 1, '/') == NULL;
}

static fs_node_t *fs_find_node_normalized(const char *normalized) {
    for (size_t i = 0; i < g_entry_count; ++i) {
        if (strcmp(g_nodes[i].name, normalized) == 0) {
            return &g_nodes[i];
        }
    }
    return NULL;
}

static const fs_node_t *fs_find_node_normalized_const(const char *normalized) {
    for (size_t i = 0; i < g_entry_count; ++i) {
        if (strcmp(g_nodes[i].name, normalized) == 0) {
            return &g_nodes[i];
        }
    }
    return NULL;
}

static bool fs_has_children(const char *normalized) {
    size_t prefix_len = strlen(normalized);
    for (size_t i = 0; i < g_entry_count; ++i) {
        if (prefix_len == 0) {
            return true;
        }
        if (strncmp(g_nodes[i].name, normalized, prefix_len) == 0 && g_nodes[i].name[prefix_len] == '/') {
            return true;
        }
    }
    return false;
}

static bool fs_parent_exists(const char *normalized) {
    char parent[FS_PATH_MAX];
    if (!fs_parent_path(normalized, parent)) {
        return false;
    }
    if (parent[0] == '\0') {
        return true;
    }
    const fs_node_t *parent_node = fs_find_node_normalized_const(parent);
    return parent_node != NULL && parent_node->type == FS_TYPE_DIR;
}

static bool fs_push_node(const char *normalized, uint32_t type) {
    if (g_entry_count >= MAX_FS_ENTRIES || strlen(normalized) + 1 > FS_PATH_MAX) {
        return false;
    }

    fs_node_t *node = &g_nodes[g_entry_count++];
    memset(node, 0, sizeof(*node));
    strcpy(node->name, normalized);
    node->type = type;
    fs_sync_entry(node);
    return true;
}

static bool fs_ensure_dir_chain(const char *normalized) {
    char partial[FS_PATH_MAX];
    size_t start = 0;

    while (normalized[start] != '\0') {
        const char *slash = strchr(normalized + start, '/');
        if (slash == NULL) {
            return true;
        }

        size_t length = (size_t)(slash - normalized);
        if (length >= FS_PATH_MAX) {
            return false;
        }

        memcpy(partial, normalized, length);
        partial[length] = '\0';

        fs_node_t *existing = fs_find_node_normalized(partial);
        if (existing == NULL) {
            if (!fs_push_node(partial, FS_TYPE_DIR)) {
                return false;
            }
        } else if (existing->type != FS_TYPE_DIR) {
            return false;
        }

        start = length + 1;
    }

    return true;
}

static bool fs_load_file(const char *path, const uint8_t *data, size_t size, uint32_t type) {
    char normalized[FS_PATH_MAX];
    if (!fs_normalize_path(path, normalized) || normalized[0] == '\0' || type == FS_TYPE_DIR || size > FS_MAX_ENTRY_SIZE) {
        return false;
    }

    if (!fs_ensure_dir_chain(normalized)) {
        return false;
    }

    if (!fs_push_node(normalized, type)) {
        return false;
    }

    fs_node_t *node = &g_nodes[g_entry_count - 1];
    memcpy(node->data, data, size);
    node->size = size;
    fs_sync_entry(node);
    return true;
}

static void fs_use_builtin(void) {
    g_entry_count = 0;
    fs_mkdir("/docs");
    fs_mkdir("/home");
    fs_write_file("/README.TXT", (const uint8_t *)builtin_readme, sizeof(builtin_readme) - 1, FS_TYPE_TEXT);
    fs_write_file("/docs/MOTD.TXT", (const uint8_t *)builtin_motd, sizeof(builtin_motd) - 1, FS_TYPE_TEXT);
    fs_write_file("/home/WELCOME.TXT", (const uint8_t *)builtin_home, sizeof(builtin_home) - 1, FS_TYPE_TEXT);
}

bool fs_init(uint32_t multiboot_magic, uintptr_t multiboot_info_addr) {
    g_entry_count = 0;

    if (multiboot_magic == MULTIBOOT_BOOTLOADER_MAGIC) {
        const multiboot_info_t *mbi = (const multiboot_info_t *)multiboot_info_addr;
        if ((mbi->flags & MULTIBOOT_INFO_MODS) != 0 && mbi->mods_count > 0) {
            const multiboot_module_t *module = (const multiboot_module_t *)mbi->mods_addr;
            const uint8_t *base = (const uint8_t *)module[0].mod_start;
            const initrd_header_t *header = (const initrd_header_t *)base;

            if (header->magic == INITRD_MAGIC && header->version == INITRD_VERSION) {
                const initrd_node_t *nodes = (const initrd_node_t *)(base + sizeof(initrd_header_t));
                size_t count = header->file_count;
                if (count > MAX_FS_ENTRIES) {
                    count = MAX_FS_ENTRIES;
                }

                for (size_t i = 0; i < count; ++i) {
                    char name[INITRD_NAME_MAX + 1];
                    size_t name_len = strnlen(nodes[i].name, sizeof(nodes[i].name));
                    memcpy(name, nodes[i].name, name_len);
                    name[name_len] = '\0';

                    if (!fs_load_file(name, base + nodes[i].offset, nodes[i].size, nodes[i].type)) {
                        break;
                    }
                }

                if (g_entry_count > 0) {
                    return true;
                }
            }
        }
    }

    fs_use_builtin();
    return false;
}

size_t fs_count(void) {
    return g_entry_count;
}

const fs_entry_t *fs_get(size_t index) {
    if (index >= g_entry_count) {
        return NULL;
    }
    return &g_nodes[index].entry;
}

const fs_entry_t *fs_find(const char *path) {
    char normalized[FS_PATH_MAX];
    if (!fs_normalize_path(path, normalized)) {
        return NULL;
    }
    const fs_node_t *node = fs_find_node_normalized_const(normalized);
    return node == NULL ? NULL : &node->entry;
}

size_t fs_list(const char *directory, const fs_entry_t **results, size_t max_results) {
    char normalized[FS_PATH_MAX];
    size_t count = 0;

    if (!fs_normalize_path(directory, normalized)) {
        return 0;
    }

    if (normalized[0] != '\0') {
        const fs_node_t *dir = fs_find_node_normalized_const(normalized);
        if (dir == NULL || dir->type != FS_TYPE_DIR) {
            return 0;
        }
    }

    for (size_t i = 0; i < g_entry_count && count < max_results; ++i) {
        if (fs_is_direct_child(normalized, g_nodes[i].name)) {
            results[count++] = &g_nodes[i].entry;
        }
    }

    return count;
}

bool fs_mkdir(const char *path) {
    char normalized[FS_PATH_MAX];
    if (!fs_normalize_path(path, normalized) || normalized[0] == '\0') {
        return false;
    }

    fs_node_t *existing = fs_find_node_normalized(normalized);
    if (existing != NULL) {
        return existing->type == FS_TYPE_DIR;
    }

    if (!fs_parent_exists(normalized)) {
        return false;
    }

    return fs_push_node(normalized, FS_TYPE_DIR);
}

bool fs_write_file(const char *path, const uint8_t *data, size_t size, uint32_t type) {
    char normalized[FS_PATH_MAX];
    if (!fs_normalize_path(path, normalized) || normalized[0] == '\0' || type == FS_TYPE_DIR || size > FS_MAX_ENTRY_SIZE) {
        return false;
    }

    fs_node_t *existing = fs_find_node_normalized(normalized);
    if (existing != NULL) {
        if (existing->type == FS_TYPE_DIR) {
            return false;
        }
        memset(existing->data, 0, sizeof(existing->data));
        if (size > 0) {
            memcpy(existing->data, data, size);
        }
        existing->size = size;
        existing->type = type;
        fs_sync_entry(existing);
        return true;
    }

    if (!fs_parent_exists(normalized) || !fs_push_node(normalized, type)) {
        return false;
    }

    fs_node_t *node = &g_nodes[g_entry_count - 1];
    if (size > 0) {
        memcpy(node->data, data, size);
    }
    node->size = size;
    fs_sync_entry(node);
    return true;
}

bool fs_append_file(const char *path, const uint8_t *data, size_t size) {
    char normalized[FS_PATH_MAX];
    if (!fs_normalize_path(path, normalized) || normalized[0] == '\0') {
        return false;
    }

    fs_node_t *existing = fs_find_node_normalized(normalized);
    if (existing == NULL) {
        return fs_write_file(normalized, data, size, FS_TYPE_TEXT);
    }
    if (existing->type != FS_TYPE_TEXT || existing->size + size > FS_MAX_ENTRY_SIZE) {
        return false;
    }

    memcpy(existing->data + existing->size, data, size);
    existing->size += size;
    fs_sync_entry(existing);
    return true;
}

bool fs_remove(const char *path) {
    char normalized[FS_PATH_MAX];
    if (!fs_normalize_path(path, normalized) || normalized[0] == '\0') {
        return false;
    }

    for (size_t i = 0; i < g_entry_count; ++i) {
        if (strcmp(g_nodes[i].name, normalized) == 0) {
            if (g_nodes[i].type == FS_TYPE_DIR && fs_has_children(normalized)) {
                return false;
            }
            for (size_t j = i + 1; j < g_entry_count; ++j) {
                g_nodes[j - 1] = g_nodes[j];
            }
            --g_entry_count;
            return true;
        }
    }

    return false;
}

bool fs_copy(const char *source, const char *destination) {
    char normalized_source[FS_PATH_MAX];
    char normalized_destination[FS_PATH_MAX];

    if (!fs_normalize_path(source, normalized_source) || !fs_normalize_path(destination, normalized_destination)) {
        return false;
    }

    const fs_node_t *node = fs_find_node_normalized_const(normalized_source);
    if (node == NULL || node->type == FS_TYPE_DIR) {
        return false;
    }
    if (fs_find_node_normalized_const(normalized_destination) != NULL) {
        return false;
    }

    return fs_write_file(normalized_destination, node->data, node->size, node->type);
}

bool fs_move(const char *source, const char *destination) {
    char normalized_source[FS_PATH_MAX];
    char normalized_destination[FS_PATH_MAX];
    size_t source_len;

    if (!fs_normalize_path(source, normalized_source) || !fs_normalize_path(destination, normalized_destination)) {
        return false;
    }
    if (normalized_source[0] == '\0' || normalized_destination[0] == '\0') {
        return false;
    }

    fs_node_t *source_node = fs_find_node_normalized(normalized_source);
    if (source_node == NULL || fs_find_node_normalized_const(normalized_destination) != NULL || !fs_parent_exists(normalized_destination)) {
        return false;
    }

    source_len = strlen(normalized_source);
    if (source_node->type == FS_TYPE_DIR &&
        strncmp(normalized_destination, normalized_source, source_len) == 0 &&
        normalized_destination[source_len] == '/') {
        return false;
    }

    for (size_t i = 0; i < g_entry_count; ++i) {
        if (strcmp(g_nodes[i].name, normalized_source) == 0 ||
            (strncmp(g_nodes[i].name, normalized_source, source_len) == 0 && g_nodes[i].name[source_len] == '/')) {
            char suffix[FS_PATH_MAX];
            char candidate[FS_PATH_MAX];
            strcpy(suffix, g_nodes[i].name + source_len);
            if (strlen(normalized_destination) + strlen(suffix) + 1 > FS_PATH_MAX) {
                return false;
            }
            strcpy(candidate, normalized_destination);
            if (suffix[0] != '\0') {
                strcat(candidate, suffix);
            }
            for (size_t j = 0; j < g_entry_count; ++j) {
                if (i != j && strcmp(g_nodes[j].name, candidate) == 0) {
                    return false;
                }
            }
        }
    }

    for (size_t i = 0; i < g_entry_count; ++i) {
        if (strcmp(g_nodes[i].name, normalized_source) == 0 ||
            (strncmp(g_nodes[i].name, normalized_source, source_len) == 0 && g_nodes[i].name[source_len] == '/')) {
            char suffix[FS_PATH_MAX];
            strcpy(suffix, g_nodes[i].name + source_len);
            strcpy(g_nodes[i].name, normalized_destination);
            if (suffix[0] != '\0') {
                strcat(g_nodes[i].name, suffix);
            }
            fs_sync_entry(&g_nodes[i]);
        }
    }

    return true;
}

bool fs_exists(const char *path) {
    return fs_find(path) != NULL;
}

bool fs_is_dir(const char *path) {
    const fs_entry_t *entry = fs_find(path);
    return entry != NULL && entry->type == FS_TYPE_DIR;
}
