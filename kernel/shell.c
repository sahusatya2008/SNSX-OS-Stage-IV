#include "ata.h"
#include "common.h"
#include "fs.h"
#include "keyboard.h"
#include "port_io.h"
#include "shell.h"
#include "string.h"
#include "task.h"
#include "terminal.h"
#include "timer.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SHELL_INPUT_MAX 256
#define SHELL_ARG_MAX 16
#define FILE_MANAGER_VIEW_MAX 32

typedef struct calc_state {
    const char *cursor;
    bool ok;
    bool divide_by_zero;
} calc_state_t;

static char g_current_directory[FS_PATH_MAX] = "/home";

static void shell_print_signed(int32_t value) {
    if (value < 0) {
        uint32_t magnitude = (uint32_t)(-(value + 1)) + 1u;
        terminal_putchar('-');
        terminal_write_dec(magnitude);
        return;
    }
    terminal_write_dec((uint32_t)value);
}

static void shell_copy_string(char *destination, const char *source, size_t limit) {
    size_t i = 0;
    if (limit == 0) {
        return;
    }
    while (source[i] != '\0' && i + 1 < limit) {
        destination[i] = source[i];
        ++i;
    }
    destination[i] = '\0';
}

static void shell_prompt(void) {
    terminal_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    terminal_write("SNSX");
    terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_write(":");
    terminal_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    terminal_write(g_current_directory);
    terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_write("> ");
}

static void shell_readline(char *buffer, size_t limit) {
    size_t len = 0;
    while (1) {
        while (!keyboard_has_char()) {
            halt();
        }

        char c = keyboard_getchar();
        if (c == '\r') {
            continue;
        }
        if (c == '\n') {
            terminal_putchar('\n');
            break;
        }
        if (c == '\b') {
            if (len > 0) {
                --len;
                terminal_backspace();
            }
            continue;
        }
        if (!isprint(c) || len + 1 >= limit) {
            continue;
        }
        buffer[len++] = c;
        terminal_putchar(c);
    }
    buffer[len] = '\0';
}

static size_t shell_tokenize(char *line, char **argv, size_t max_args) {
    size_t argc = 0;
    char *cursor = line;

    while (*cursor != '\0' && argc < max_args) {
        while (isspace(*cursor)) {
            ++cursor;
        }
        if (*cursor == '\0') {
            break;
        }
        argv[argc++] = cursor;
        while (*cursor != '\0' && !isspace(*cursor)) {
            ++cursor;
        }
        if (*cursor == '\0') {
            break;
        }
        *cursor++ = '\0';
    }

    return argc;
}

static void shell_join_args(size_t start, size_t argc, char **argv, char *output, size_t limit) {
    size_t out = 0;

    if (limit == 0) {
        return;
    }

    output[0] = '\0';
    for (size_t i = start; i < argc; ++i) {
        size_t j = 0;
        if (i > start && out + 1 < limit) {
            output[out++] = ' ';
        }
        while (argv[i][j] != '\0' && out + 1 < limit) {
            output[out++] = argv[i][j++];
        }
    }
    output[out] = '\0';
}

static void shell_wait_for_enter(const char *message) {
    char temp[4];
    if (message != NULL) {
        terminal_writeline(message);
    }
    terminal_write("Press Enter to continue...");
    shell_readline(temp, sizeof(temp));
}

static void shell_parent_directory(char *path) {
    char *slash;
    if (strcmp(path, "/") == 0) {
        return;
    }
    slash = strrchr(path, '/');
    if (slash == path) {
        path[1] = '\0';
        return;
    }
    if (slash != NULL) {
        *slash = '\0';
    }
}

static const char *shell_basename(const char *path) {
    const char *slash = strrchr(path, '/');
    return slash == NULL ? path : slash + 1;
}

static void shell_entry_absolute_path(const fs_entry_t *entry, char *output) {
    output[0] = '/';
    shell_copy_string(output + 1, entry->name, FS_PATH_MAX - 1);
}

static void shell_resolve_path_from(const char *current_directory, const char *input, char *output) {
    char raw[FS_PATH_MAX];
    char components[16][FS_PATH_MAX];
    size_t depth = 0;
    size_t raw_len = 0;
    size_t i = 0;
    size_t out = 0;

    if (input == NULL || input[0] == '\0') {
        shell_copy_string(output, current_directory, FS_PATH_MAX);
        return;
    }

    memset(raw, 0, sizeof(raw));
    if (input[0] == '/') {
        shell_copy_string(raw, input, sizeof(raw));
    } else if (strcmp(current_directory, "/") == 0) {
        raw[raw_len++] = '/';
        shell_copy_string(raw + raw_len, input, sizeof(raw) - raw_len);
    } else {
        shell_copy_string(raw, current_directory, sizeof(raw));
        raw_len = strlen(raw);
        if (raw_len + 1 < sizeof(raw)) {
            raw[raw_len++] = '/';
            raw[raw_len] = '\0';
        }
        shell_copy_string(raw + raw_len, input, sizeof(raw) - raw_len);
    }

    while (raw[i] != '\0') {
        char token[FS_PATH_MAX];
        size_t token_len = 0;

        while (raw[i] == '/') {
            ++i;
        }
        if (raw[i] == '\0') {
            break;
        }

        while (raw[i] != '\0' && raw[i] != '/' && token_len + 1 < sizeof(token)) {
            token[token_len++] = raw[i++];
        }
        token[token_len] = '\0';

        if (strcmp(token, ".") == 0 || token_len == 0) {
            continue;
        }
        if (strcmp(token, "..") == 0) {
            if (depth > 0) {
                --depth;
            }
            continue;
        }
        if (depth < ARRAY_SIZE(components)) {
            shell_copy_string(components[depth++], token, FS_PATH_MAX);
        }
    }

    output[out++] = '/';
    if (depth == 0) {
        output[out] = '\0';
        return;
    }

    for (size_t component = 0; component < depth; ++component) {
        size_t j = 0;
        while (components[component][j] != '\0' && out + 1 < FS_PATH_MAX) {
            output[out++] = components[component][j++];
        }
        if (component + 1 < depth && out + 1 < FS_PATH_MAX) {
            output[out++] = '/';
        }
    }
    output[out] = '\0';
}

static void shell_resolve_path(const char *input, char *output) {
    shell_resolve_path_from(g_current_directory, input, output);
}

static void shell_print_path(const char *path_without_leading_slash) {
    terminal_putchar('/');
    terminal_write(path_without_leading_slash);
}

static void shell_print_entry_line(const fs_entry_t *entry) {
    const char *name = shell_basename(entry->name);
    if (entry->type == FS_TYPE_DIR) {
        terminal_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
        terminal_write(name);
        terminal_write("/");
        terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        terminal_writeline("  <dir>");
        return;
    }

    terminal_write(name);
    terminal_write("  ");
    terminal_write_dec((uint32_t)entry->size);
    terminal_write(" bytes  ");
    terminal_writeline(entry->type == FS_TYPE_PROGRAM ? "program" : "text");
}

static void shell_preview_text(const char *absolute_path) {
    const fs_entry_t *entry = fs_find(absolute_path);
    terminal_clear();
    if (entry == NULL) {
        terminal_writeline("Path not found.");
        shell_wait_for_enter(NULL);
        return;
    }

    terminal_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    terminal_write("SNSX Viewer: ");
    terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_writeline(absolute_path);
    terminal_writeline("");

    if (entry->type == FS_TYPE_DIR) {
        terminal_writeline("This path is a directory.");
    } else if (entry->type == FS_TYPE_PROGRAM) {
        terminal_writeline("This file is a program binary. Use run PATH to execute it.");
    } else {
        for (size_t i = 0; i < entry->size; ++i) {
            terminal_putchar((char)entry->data[i]);
        }
        if (entry->size == 0 || entry->data[entry->size - 1] != '\n') {
            terminal_putchar('\n');
        }
    }

    shell_wait_for_enter(NULL);
}

static void shell_tree_walk(const char *absolute_path, int depth) {
    const fs_entry_t *entries[FILE_MANAGER_VIEW_MAX];
    size_t count = fs_list(absolute_path, entries, ARRAY_SIZE(entries));
    for (size_t i = 0; i < count; ++i) {
        char child_path[FS_PATH_MAX];
        for (int indent = 0; indent < depth; ++indent) {
            terminal_write("  ");
        }
        shell_print_entry_line(entries[i]);
        if (entries[i]->type == FS_TYPE_DIR) {
            shell_entry_absolute_path(entries[i], child_path);
            shell_tree_walk(child_path, depth + 1);
        }
    }
}

static void shell_stat_path(const char *absolute_path) {
    const fs_entry_t *entry = fs_find(absolute_path);
    if (entry == NULL) {
        terminal_writeline("Path not found.");
        return;
    }
    terminal_write("Path: ");
    shell_print_path(entry->name);
    terminal_writeline("");
    terminal_write("Type: ");
    if (entry->type == FS_TYPE_DIR) {
        terminal_writeline("directory");
    } else if (entry->type == FS_TYPE_PROGRAM) {
        terminal_writeline("program");
    } else {
        terminal_writeline("text");
    }
    terminal_write("Size: ");
    terminal_write_dec((uint32_t)entry->size);
    terminal_writeline(" bytes");
}

static void calc_skip_spaces(calc_state_t *state) {
    while (isspace(*state->cursor)) {
        ++state->cursor;
    }
}

static int32_t calc_parse_expression(calc_state_t *state);

static int32_t calc_parse_factor(calc_state_t *state) {
    int32_t value = 0;
    bool negative = false;

    calc_skip_spaces(state);
    while (*state->cursor == '+' || *state->cursor == '-') {
        if (*state->cursor == '-') {
            negative = !negative;
        }
        ++state->cursor;
        calc_skip_spaces(state);
    }

    if (*state->cursor == '(') {
        ++state->cursor;
        value = calc_parse_expression(state);
        calc_skip_spaces(state);
        if (*state->cursor != ')') {
            state->ok = false;
            return 0;
        }
        ++state->cursor;
        return negative ? -value : value;
    }

    if (*state->cursor < '0' || *state->cursor > '9') {
        state->ok = false;
        return 0;
    }

    while (*state->cursor >= '0' && *state->cursor <= '9') {
        value = value * 10 + (*state->cursor - '0');
        ++state->cursor;
    }
    return negative ? -value : value;
}

static int32_t calc_parse_term(calc_state_t *state) {
    int32_t value = calc_parse_factor(state);

    while (state->ok) {
        calc_skip_spaces(state);
        if (*state->cursor != '*' && *state->cursor != '/') {
            break;
        }

        char op = *state->cursor++;
        int32_t rhs = calc_parse_factor(state);
        if (!state->ok) {
            return 0;
        }

        if (op == '*') {
            value *= rhs;
        } else {
            if (rhs == 0) {
                state->ok = false;
                state->divide_by_zero = true;
                return 0;
            }
            value /= rhs;
        }
    }

    return value;
}

static int32_t calc_parse_expression(calc_state_t *state) {
    int32_t value = calc_parse_term(state);

    while (state->ok) {
        calc_skip_spaces(state);
        if (*state->cursor != '+' && *state->cursor != '-') {
            break;
        }

        char op = *state->cursor++;
        int32_t rhs = calc_parse_term(state);
        if (!state->ok) {
            return 0;
        }
        if (op == '+') {
            value += rhs;
        } else {
            value -= rhs;
        }
    }

    return value;
}

static bool calc_evaluate(const char *expression, int32_t *result) {
    calc_state_t state;
    state.cursor = expression;
    state.ok = true;
    state.divide_by_zero = false;

    *result = calc_parse_expression(&state);
    calc_skip_spaces(&state);
    if (!state.ok || *state.cursor != '\0') {
        return false;
    }
    return true;
}

static void app_calculator(size_t argc, char **argv) {
    char expression[SHELL_INPUT_MAX];

    if (argc > 1) {
        int32_t result;
        shell_join_args(1, argc, argv, expression, sizeof(expression));
        if (!calc_evaluate(expression, &result)) {
            terminal_writeline("Invalid expression. Example: calc (2 + 3) * 4");
            return;
        }
        terminal_write("Result: ");
        shell_print_signed(result);
        terminal_writeline("");
        return;
    }

    terminal_writeline("SNSX Calculator");
    terminal_writeline("Type an expression or 'exit'. Supported operators: + - * / and parentheses.");
    while (1) {
        terminal_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
        terminal_write("calc");
        terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        terminal_write("> ");
        shell_readline(expression, sizeof(expression));
        if (strcmp(expression, "exit") == 0 || strcmp(expression, "quit") == 0) {
            return;
        }
        if (expression[0] == '\0') {
            continue;
        }

        int32_t result;
        if (!calc_evaluate(expression, &result)) {
            terminal_writeline("Invalid expression.");
            continue;
        }
        terminal_write("= ");
        shell_print_signed(result);
        terminal_writeline("");
    }
}

static void app_text_editor(const char *absolute_path) {
    char buffer[FS_MAX_ENTRY_SIZE];
    char line[SHELL_INPUT_MAX];
    size_t length = 0;
    const fs_entry_t *entry = fs_find(absolute_path);

    memset(buffer, 0, sizeof(buffer));

    if (entry != NULL) {
        if (entry->type == FS_TYPE_DIR) {
            terminal_writeline("Cannot edit a directory.");
            return;
        }
        if (entry->type == FS_TYPE_PROGRAM) {
            terminal_writeline("Cannot edit a program binary as text.");
            return;
        }
        if (entry->size > sizeof(buffer)) {
            terminal_writeline("File is too large for the editor buffer.");
            return;
        }
        memcpy(buffer, entry->data, entry->size);
        length = entry->size;
    }

    terminal_clear();
    terminal_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    terminal_write("SNSX Editor: ");
    terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_writeline(absolute_path);
    terminal_writeline("Line editor commands: .save  .quit  .show  .clear  .help");
    terminal_writeline("Typed lines are appended to the current buffer. Use .clear to rewrite from scratch.");
    terminal_writeline("");

    if (length > 0) {
        terminal_writeline("Existing content:");
        for (size_t i = 0; i < length; ++i) {
            terminal_putchar((char)buffer[i]);
        }
        if (buffer[length - 1] != '\n') {
            terminal_putchar('\n');
        }
        terminal_writeline("");
    }

    while (1) {
        terminal_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        terminal_write("edit");
        terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        terminal_write("> ");
        shell_readline(line, sizeof(line));

        if (strcmp(line, ".help") == 0) {
            terminal_writeline(".save  Save and leave the editor");
            terminal_writeline(".quit  Leave without saving");
            terminal_writeline(".show  Display the current buffer");
            terminal_writeline(".clear Clear the current buffer");
            continue;
        }
        if (strcmp(line, ".quit") == 0) {
            terminal_writeline("Editor closed without saving.");
            return;
        }
        if (strcmp(line, ".clear") == 0) {
            memset(buffer, 0, sizeof(buffer));
            length = 0;
            terminal_writeline("Buffer cleared.");
            continue;
        }
        if (strcmp(line, ".show") == 0) {
            terminal_writeline("-----");
            if (length == 0) {
                terminal_writeline("<empty>");
            } else {
                for (size_t i = 0; i < length; ++i) {
                    terminal_putchar((char)buffer[i]);
                }
                if (buffer[length - 1] != '\n') {
                    terminal_putchar('\n');
                }
            }
            terminal_writeline("-----");
            continue;
        }
        if (strcmp(line, ".save") == 0) {
            if (!fs_write_file(absolute_path, (const uint8_t *)buffer, length, FS_TYPE_TEXT)) {
                terminal_writeline("Save failed. Check that the parent directory exists.");
                continue;
            }
            terminal_writeline("File saved.");
            return;
        }

        size_t line_length = strlen(line);
        if (length + line_length + 1 >= sizeof(buffer)) {
            terminal_writeline("Editor buffer is full.");
            continue;
        }
        memcpy(buffer + length, line, line_length);
        length += line_length;
        buffer[length++] = '\n';
        buffer[length] = '\0';
    }
}

static bool shell_parse_index(const char *text, size_t max_count, size_t *index) {
    int parsed = atoi(text);
    if (parsed <= 0 || (size_t)parsed > max_count) {
        return false;
    }
    *index = (size_t)parsed - 1;
    return true;
}

static void app_file_manager(void) {
    char manager_cwd[FS_PATH_MAX];
    char line[SHELL_INPUT_MAX];
    char *argv[SHELL_ARG_MAX];
    const fs_entry_t *entries[FILE_MANAGER_VIEW_MAX];

    shell_copy_string(manager_cwd, g_current_directory, sizeof(manager_cwd));

    while (1) {
        terminal_clear();
        terminal_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
        terminal_writeline("SNSX File Manager");
        terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        terminal_write("Directory: ");
        terminal_writeline(manager_cwd);
        terminal_writeline("");

        size_t count = fs_list(manager_cwd, entries, ARRAY_SIZE(entries));
        if (count == 0) {
            terminal_writeline("<empty>");
        } else {
            for (size_t i = 0; i < count; ++i) {
                terminal_write_dec((uint32_t)(i + 1));
                terminal_write(". ");
                shell_print_entry_line(entries[i]);
            }
        }

        terminal_writeline("");
        terminal_writeline("Commands: open N, edit N, run N, cd N, del N, mkdir NAME, new NAME, ren N NAME, copy N NAME, up, quit");
        terminal_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        terminal_write("files");
        terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        terminal_write("> ");
        shell_readline(line, sizeof(line));

        size_t argc = shell_tokenize(line, argv, ARRAY_SIZE(argv));
        if (argc == 0) {
            continue;
        }
        if (strcmp(argv[0], "quit") == 0 || strcmp(argv[0], "exit") == 0) {
            shell_copy_string(g_current_directory, manager_cwd, sizeof(g_current_directory));
            return;
        }
        if (strcmp(argv[0], "up") == 0) {
            shell_parent_directory(manager_cwd);
            continue;
        }

        if (argc < 2) {
            terminal_writeline("More input is needed for that command.");
            shell_wait_for_enter(NULL);
            continue;
        }

        size_t index = 0;
        if ((strcmp(argv[0], "open") == 0 || strcmp(argv[0], "edit") == 0 || strcmp(argv[0], "run") == 0 ||
             strcmp(argv[0], "cd") == 0 || strcmp(argv[0], "del") == 0 || strcmp(argv[0], "ren") == 0 ||
             strcmp(argv[0], "copy") == 0) &&
            !shell_parse_index(argv[1], count, &index)) {
            terminal_writeline("Invalid entry number.");
            shell_wait_for_enter(NULL);
            continue;
        }

        if (strcmp(argv[0], "open") == 0) {
            char absolute_path[FS_PATH_MAX];
            shell_entry_absolute_path(entries[index], absolute_path);
            if (entries[index]->type == FS_TYPE_DIR) {
                shell_copy_string(manager_cwd, absolute_path, sizeof(manager_cwd));
            } else {
                shell_preview_text(absolute_path);
            }
            continue;
        }

        if (strcmp(argv[0], "edit") == 0) {
            char absolute_path[FS_PATH_MAX];
            shell_entry_absolute_path(entries[index], absolute_path);
            app_text_editor(absolute_path);
            continue;
        }

        if (strcmp(argv[0], "run") == 0) {
            char absolute_path[FS_PATH_MAX];
            shell_entry_absolute_path(entries[index], absolute_path);
            const fs_entry_t *entry_to_run = fs_find(absolute_path);
            if (entry_to_run == NULL || entry_to_run->type != FS_TYPE_PROGRAM) {
                terminal_writeline("That entry is not runnable.");
                shell_wait_for_enter(NULL);
                continue;
            }
            int pid = task_run_program(entry_to_run->name, entry_to_run->data, entry_to_run->size);
            terminal_write("Program finished with pid ");
            terminal_write_dec((uint32_t)pid);
            terminal_writeline(".");
            shell_wait_for_enter(NULL);
            continue;
        }

        if (strcmp(argv[0], "cd") == 0) {
            if (entries[index]->type != FS_TYPE_DIR) {
                terminal_writeline("That entry is not a directory.");
                shell_wait_for_enter(NULL);
                continue;
            }
            shell_entry_absolute_path(entries[index], manager_cwd);
            continue;
        }

        if (strcmp(argv[0], "del") == 0) {
            char absolute_path[FS_PATH_MAX];
            shell_entry_absolute_path(entries[index], absolute_path);
            if (!fs_remove(absolute_path)) {
                terminal_writeline("Delete failed. Directories must be empty.");
                shell_wait_for_enter(NULL);
            }
            continue;
        }

        if (strcmp(argv[0], "mkdir") == 0) {
            char absolute_path[FS_PATH_MAX];
            shell_resolve_path_from(manager_cwd, argv[1], absolute_path);
            if (!fs_mkdir(absolute_path)) {
                terminal_writeline("Directory creation failed.");
                shell_wait_for_enter(NULL);
            }
            continue;
        }

        if (strcmp(argv[0], "new") == 0) {
            char absolute_path[FS_PATH_MAX];
            shell_resolve_path_from(manager_cwd, argv[1], absolute_path);
            if (!fs_write_file(absolute_path, (const uint8_t *)"", 0, FS_TYPE_TEXT)) {
                terminal_writeline("Could not create the file.");
                shell_wait_for_enter(NULL);
                continue;
            }
            app_text_editor(absolute_path);
            continue;
        }

        if (strcmp(argv[0], "ren") == 0 || strcmp(argv[0], "copy") == 0) {
            char source_path[FS_PATH_MAX];
            char destination_path[FS_PATH_MAX];
            char parent[FS_PATH_MAX];

            if (argc < 3) {
                terminal_writeline("A new name is required.");
                shell_wait_for_enter(NULL);
                continue;
            }

            shell_entry_absolute_path(entries[index], source_path);
            shell_copy_string(parent, source_path, sizeof(parent));
            shell_parent_directory(parent);
            if (strcmp(parent, "/") == 0) {
                shell_copy_string(destination_path, "/", sizeof(destination_path));
                if (strlen(destination_path) + strlen(argv[2]) + 1 < sizeof(destination_path)) {
                    strcat(destination_path, argv[2]);
                }
            } else {
                shell_copy_string(destination_path, parent, sizeof(destination_path));
                if (strlen(destination_path) + strlen(argv[2]) + 2 < sizeof(destination_path)) {
                    strcat(destination_path, "/");
                    strcat(destination_path, argv[2]);
                }
            }

            if (strcmp(argv[0], "ren") == 0) {
                if (!fs_move(source_path, destination_path)) {
                    terminal_writeline("Rename failed.");
                    shell_wait_for_enter(NULL);
                }
            } else {
                if (!fs_copy(source_path, destination_path)) {
                    terminal_writeline("Copy failed.");
                    shell_wait_for_enter(NULL);
                }
            }
            continue;
        }

        terminal_writeline("Unknown file manager command.");
        shell_wait_for_enter(NULL);
    }
}

static void command_help(void) {
    terminal_writeline("help             Show available SNSX commands");
    terminal_writeline("apps             Show bundled apps and launch tips");
    terminal_writeline("clear            Clear the screen");
    terminal_writeline("pwd              Show the current directory");
    terminal_writeline("ls [PATH]        List files or directories");
    terminal_writeline("tree [PATH]      Show a directory tree");
    terminal_writeline("cd DIR           Change current directory");
    terminal_writeline("cat FILE         View a text file");
    terminal_writeline("stat PATH        Show file metadata");
    terminal_writeline("touch FILE       Create an empty text file");
    terminal_writeline("mkdir DIR        Create a directory");
    terminal_writeline("rm PATH          Remove a file or empty directory");
    terminal_writeline("cp SRC DST       Copy a text/program file");
    terminal_writeline("mv SRC DST       Move or rename a file/directory");
    terminal_writeline("files            Launch the file manager app");
    terminal_writeline("calc [EXPR]      Launch calculator or evaluate an expression");
    terminal_writeline("edit FILE        Launch the text editor app");
    terminal_writeline("run FILE         Execute a bundled flat program");
    terminal_writeline("ps               Show tracked tasks");
    terminal_writeline("ticks            Show PIT tick counter");
    terminal_writeline("ata              Probe the primary ATA disk");
    terminal_writeline("about            Show system information");
    terminal_writeline("reboot           Reset the machine");
}

static void command_apps(void) {
    terminal_writeline("SNSX apps:");
    terminal_writeline("files            Full-screen file manager");
    terminal_writeline("calc             Calculator app with expression parsing");
    terminal_writeline("edit FILE        Text editor app");
    terminal_writeline("run /bin/hello.app");
    terminal_writeline("run /bin/clear.app");
}

static void command_pwd(void) {
    terminal_writeline(g_current_directory);
}

static void command_ls(const char *path) {
    char absolute_path[FS_PATH_MAX];
    const fs_entry_t *entries[FILE_MANAGER_VIEW_MAX];
    const fs_entry_t *entry;

    shell_resolve_path(path, absolute_path);
    entry = fs_find(absolute_path);
    if (entry != NULL && entry->type != FS_TYPE_DIR) {
        shell_print_entry_line(entry);
        return;
    }
    if (entry == NULL && strcmp(absolute_path, "/") != 0) {
        terminal_writeline("Path not found.");
        return;
    }

    size_t count = fs_list(absolute_path, entries, ARRAY_SIZE(entries));
    if (count == 0) {
        terminal_writeline("<empty>");
        return;
    }

    for (size_t i = 0; i < count; ++i) {
        shell_print_entry_line(entries[i]);
    }
}

static void command_tree(const char *path) {
    char absolute_path[FS_PATH_MAX];
    const fs_entry_t *entry;

    shell_resolve_path(path, absolute_path);
    entry = fs_find(absolute_path);
    if (entry == NULL) {
        if (strcmp(absolute_path, "/") == 0) {
            terminal_writeline("/");
            shell_tree_walk(absolute_path, 1);
            return;
        }
        terminal_writeline("Path not found.");
        return;
    }
    if (entry->type != FS_TYPE_DIR) {
        shell_print_entry_line(entry);
        return;
    }

    terminal_writeline(absolute_path);
    shell_tree_walk(absolute_path, 1);
}

static void command_cd(const char *path) {
    char absolute_path[FS_PATH_MAX];
    shell_resolve_path(path, absolute_path);
    if (!fs_is_dir(absolute_path)) {
        terminal_writeline("Directory not found.");
        return;
    }
    shell_copy_string(g_current_directory, absolute_path, sizeof(g_current_directory));
}

static void command_cat(const char *path) {
    char absolute_path[FS_PATH_MAX];
    const fs_entry_t *entry;

    shell_resolve_path(path, absolute_path);
    entry = fs_find(absolute_path);
    if (entry == NULL) {
        terminal_writeline("File not found.");
        return;
    }
    if (entry->type != FS_TYPE_TEXT) {
        terminal_writeline("Use stat or run depending on the file type.");
        return;
    }
    for (size_t i = 0; i < entry->size; ++i) {
        terminal_putchar((char)entry->data[i]);
    }
    if (entry->size == 0 || entry->data[entry->size - 1] != '\n') {
        terminal_putchar('\n');
    }
}

static void command_touch(const char *path) {
    char absolute_path[FS_PATH_MAX];
    shell_resolve_path(path, absolute_path);
    if (!fs_write_file(absolute_path, (const uint8_t *)"", 0, FS_TYPE_TEXT)) {
        terminal_writeline("File creation failed.");
    }
}

static void command_mkdir(const char *path) {
    char absolute_path[FS_PATH_MAX];
    shell_resolve_path(path, absolute_path);
    if (!fs_mkdir(absolute_path)) {
        terminal_writeline("Directory creation failed.");
    }
}

static void command_remove(const char *path) {
    char absolute_path[FS_PATH_MAX];
    shell_resolve_path(path, absolute_path);
    if (!fs_remove(absolute_path)) {
        terminal_writeline("Remove failed. Directories must be empty.");
    }
}

static void command_copy(const char *source, const char *destination) {
    char absolute_source[FS_PATH_MAX];
    char absolute_destination[FS_PATH_MAX];
    shell_resolve_path(source, absolute_source);
    shell_resolve_path(destination, absolute_destination);
    if (!fs_copy(absolute_source, absolute_destination)) {
        terminal_writeline("Copy failed.");
    }
}

static void command_move(const char *source, const char *destination) {
    char absolute_source[FS_PATH_MAX];
    char absolute_destination[FS_PATH_MAX];
    shell_resolve_path(source, absolute_source);
    shell_resolve_path(destination, absolute_destination);
    if (!fs_move(absolute_source, absolute_destination)) {
        terminal_writeline("Move failed.");
    }
}

static void command_edit(const char *path) {
    char absolute_path[FS_PATH_MAX];
    shell_resolve_path(path, absolute_path);
    app_text_editor(absolute_path);
}

static void command_run(const char *path) {
    char absolute_path[FS_PATH_MAX];
    const fs_entry_t *entry;

    shell_resolve_path(path, absolute_path);
    entry = fs_find(absolute_path);
    if (entry == NULL) {
        terminal_writeline("Program not found.");
        return;
    }
    if (entry->type != FS_TYPE_PROGRAM) {
        terminal_writeline("That path is not a runnable program.");
        return;
    }

    int pid = task_run_program(entry->name, entry->data, entry->size);
    if (pid < 0) {
        terminal_writeline("Program launch failed.");
        return;
    }
    terminal_write("Program completed, pid ");
    terminal_write_dec((uint32_t)pid);
    terminal_writeline("");
}

static void command_ps(void) {
    size_t count = 0;
    const task_t *tasks = task_table(&count);
    terminal_writeline("PID  STATE     TICKS  NAME");
    for (size_t i = 0; i < count; ++i) {
        terminal_write_dec((uint32_t)tasks[i].pid);
        terminal_write("    ");
        if (tasks[i].state == TASK_RUNNING) {
            terminal_write("running");
        } else if (tasks[i].state == TASK_READY) {
            terminal_write("ready  ");
        } else {
            terminal_write("zombie ");
        }
        terminal_write("   ");
        terminal_write_dec(tasks[i].runtime_ticks);
        terminal_write("    ");
        terminal_writeline(tasks[i].name);
    }
}

static void command_ata(void) {
    ata_identity_t identity;
    if (!ata_identify_primary(&identity)) {
        terminal_writeline("No primary ATA device detected.");
        return;
    }
    terminal_write("Model: ");
    terminal_writeline(identity.model);
    terminal_write("LBA28 sectors: ");
    terminal_write_dec(identity.lba28_sectors);
    terminal_writeline("");
}

static void command_about(void) {
    terminal_writeline("SNSX 0.2 live operating system");
    terminal_writeline("32-bit x86 monolithic kernel written in C and NASM.");
    terminal_writeline("Features: paging, interrupts, keyboard IRQs, PIT, ATA, writable RAM VFS, shell apps.");
    terminal_writeline("Runtime file changes live in memory for the current boot session.");
}

static void command_reboot(void) {
    outb(0x64, 0xFE);
    while (1) {
        halt();
    }
}

void shell_run(void) {
    char line[SHELL_INPUT_MAX];
    char *argv[SHELL_ARG_MAX];

    if (!fs_is_dir(g_current_directory)) {
        shell_copy_string(g_current_directory, "/", sizeof(g_current_directory));
    }

    terminal_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    terminal_writeline("SNSX live shell ready.");
    terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_writeline("Apps: files, calc, edit FILE. Type help for the full command set.");

    while (1) {
        shell_prompt();
        shell_readline(line, sizeof(line));

        size_t argc = shell_tokenize(line, argv, ARRAY_SIZE(argv));
        if (argc == 0) {
            continue;
        }

        if (strcmp(argv[0], "help") == 0) {
            command_help();
        } else if (strcmp(argv[0], "apps") == 0) {
            command_apps();
        } else if (strcmp(argv[0], "clear") == 0) {
            terminal_clear();
        } else if (strcmp(argv[0], "pwd") == 0) {
            command_pwd();
        } else if (strcmp(argv[0], "ls") == 0) {
            command_ls(argc > 1 ? argv[1] : g_current_directory);
        } else if (strcmp(argv[0], "tree") == 0) {
            command_tree(argc > 1 ? argv[1] : g_current_directory);
        } else if (strcmp(argv[0], "cd") == 0) {
            if (argc < 2) {
                terminal_writeline("Usage: cd DIR");
            } else {
                command_cd(argv[1]);
            }
        } else if (strcmp(argv[0], "cat") == 0) {
            if (argc < 2) {
                terminal_writeline("Usage: cat FILE");
            } else {
                command_cat(argv[1]);
            }
        } else if (strcmp(argv[0], "stat") == 0) {
            char absolute_path[FS_PATH_MAX];
            if (argc < 2) {
                terminal_writeline("Usage: stat PATH");
            } else {
                shell_resolve_path(argv[1], absolute_path);
                shell_stat_path(absolute_path);
            }
        } else if (strcmp(argv[0], "touch") == 0) {
            if (argc < 2) {
                terminal_writeline("Usage: touch FILE");
            } else {
                command_touch(argv[1]);
            }
        } else if (strcmp(argv[0], "mkdir") == 0) {
            if (argc < 2) {
                terminal_writeline("Usage: mkdir DIR");
            } else {
                command_mkdir(argv[1]);
            }
        } else if (strcmp(argv[0], "rm") == 0) {
            if (argc < 2) {
                terminal_writeline("Usage: rm PATH");
            } else {
                command_remove(argv[1]);
            }
        } else if (strcmp(argv[0], "cp") == 0) {
            if (argc < 3) {
                terminal_writeline("Usage: cp SRC DST");
            } else {
                command_copy(argv[1], argv[2]);
            }
        } else if (strcmp(argv[0], "mv") == 0) {
            if (argc < 3) {
                terminal_writeline("Usage: mv SRC DST");
            } else {
                command_move(argv[1], argv[2]);
            }
        } else if (strcmp(argv[0], "files") == 0) {
            app_file_manager();
            terminal_clear();
        } else if (strcmp(argv[0], "calc") == 0) {
            app_calculator(argc, argv);
        } else if (strcmp(argv[0], "edit") == 0) {
            if (argc < 2) {
                terminal_writeline("Usage: edit FILE");
            } else {
                command_edit(argv[1]);
            }
        } else if (strcmp(argv[0], "run") == 0) {
            if (argc < 2) {
                terminal_writeline("Usage: run FILE");
            } else {
                command_run(argv[1]);
            }
        } else if (strcmp(argv[0], "ps") == 0) {
            command_ps();
        } else if (strcmp(argv[0], "ticks") == 0) {
            terminal_write("Ticks: ");
            terminal_write_dec(timer_ticks());
            terminal_writeline("");
        } else if (strcmp(argv[0], "ata") == 0) {
            command_ata();
        } else if (strcmp(argv[0], "about") == 0) {
            command_about();
        } else if (strcmp(argv[0], "reboot") == 0) {
            command_reboot();
        } else {
            terminal_writeline("Unknown command. Type help.");
        }
    }
}
