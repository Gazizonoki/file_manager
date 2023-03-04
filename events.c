#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "events.h"

enum {
    SIZE_SECTION_LEN = 30,
    NAME_SECTION_LEN = 30
};

enum color_id {
    HEADER = 1,
    FILE_NAME = 2,
    DIR_NAME = 3,
    FIFO_NAME = 4,
    LNK_NAME = 5,
    SIZE = 6,
    FILE_NAME_HIGHLIGHTED = 7,
    DIR_NAME_HIGHLIGHTED = 8,
    FIFO_NAME_HIGHLIGHTED = 9,
    LNK_NAME_HIGHLIGHTED = 10,
    SIZE_HIGHLIHTED = 11
};

static WINDOW *screen;
static int str_count, col_count;
static size_t current_line = 0, start_line = 1;
static char *header_str;
static DIR *cur_dir;
static char *data_file_path = NULL;
static char *data_file_name = NULL;
static size_t data_len;
static bool is_extract = false, show_hidden_files = false;

static bool is_normal_entry(char *filename) {
    if (strcmp(filename, ".") == 0) {
        return false;
    }
    if (strcmp(filename, "..") == 0) {
        return true;
    }
    if (!show_hidden_files && filename[0] == '.') {
        return false;
    }
    return true;
}

void clear_data() {
    free(data_file_path);
    free(data_file_name);
    endwin();
}

static struct dirent *get_cur_entry() {
    cur_dir = opendir(".");
    struct dirent *entry;
    size_t line = 1;
    while (line <= current_line) {
        entry = readdir(cur_dir);
        if (is_normal_entry(entry->d_name)) {
            ++line;
        }
    }
    return entry;
}

static void close_cur_dir() {
    closedir(cur_dir);
}

void init_colors() {
    if (!has_colors()) {
        clear_data();
        fprintf(stderr, "Colors aren't supported in your terminal\n");
        exit(EXIT_FAILURE);
    }
    start_color();
    init_pair(HEADER, COLOR_BLACK, COLOR_WHITE);
    init_pair(FILE_NAME, COLOR_WHITE, COLOR_BLACK);
    init_pair(DIR_NAME, COLOR_BLUE, COLOR_BLACK);
    init_pair(FIFO_NAME, COLOR_YELLOW, COLOR_BLACK);
    init_pair(LNK_NAME, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(SIZE, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(FILE_NAME_HIGHLIGHTED, COLOR_WHITE, COLOR_CYAN);
    init_pair(DIR_NAME_HIGHLIGHTED, COLOR_BLUE, COLOR_CYAN);
    init_pair(FIFO_NAME_HIGHLIGHTED, COLOR_YELLOW, COLOR_CYAN);
    init_pair(LNK_NAME_HIGHLIGHTED, COLOR_MAGENTA, COLOR_CYAN);
    init_pair(SIZE_HIGHLIHTED, COLOR_MAGENTA, COLOR_CYAN);
}

static char name_str[NAME_SECTION_LEN + 1];
static char size_str[SIZE_SECTION_LEN + 1];

static void print_line(struct dirent *entry, size_t *line) {
    if (!is_normal_entry(entry->d_name)) {
        return;
    }
    int len = snprintf(name_str, NAME_SECTION_LEN + 1, "%s", entry->d_name);
    if (len > NAME_SECTION_LEN) {
        len = NAME_SECTION_LEN;
    }
    short name_color_pair, size_color_pair;
    if (*line == current_line) {
        if (entry->d_type == DT_DIR) {
            name_color_pair = DIR_NAME_HIGHLIGHTED;
        } else if (entry->d_type == DT_FIFO) {
            name_color_pair = FIFO_NAME_HIGHLIGHTED;
        } else if (entry->d_type == DT_LNK) {
            name_color_pair = LNK_NAME_HIGHLIGHTED;
        } else {
            name_color_pair = FILE_NAME_HIGHLIGHTED;
        }
        size_color_pair = SIZE_HIGHLIHTED;
    } else {
        if (entry->d_type == DT_DIR) {
            name_color_pair = DIR_NAME;
        } else if (entry->d_type == DT_FIFO) {
            name_color_pair = FIFO_NAME;
        } else if (entry->d_type == DT_LNK) {
            name_color_pair = LNK_NAME;
        } else {
            name_color_pair = FILE_NAME;
        }
        size_color_pair = SIZE;
    }

    attron(COLOR_PAIR(name_color_pair));
    addstr(name_str);
    attron(COLOR_PAIR(size_color_pair));
    while (len + SIZE_SECTION_LEN + 1 < col_count) {
        addch(' ');
        ++len;
    }
    struct stat info;
    stat(entry->d_name, &info);
    len = snprintf(size_str, SIZE_SECTION_LEN + 1, "%jd", info.st_size);
    while (len < SIZE_SECTION_LEN + 1) {
        addch(' ');
        ++len;
    }
    addstr(size_str);
    attroff(COLOR_PAIR(size_color_pair));
    ++(*line);
}

void print_dirs(void) {
    clear();
    DIR *dir = opendir(".");
    if (!dir) {
        clear_data();
        fprintf(stderr, "Couldn't open directory");
        exit(EXIT_FAILURE);
    }
    struct dirent *entry;

    size_t line = 1;
    attron(COLOR_PAIR(HEADER));
    addstr(header_str);
    attroff(COLOR_PAIR(HEADER));

    while (line < start_line) {
        entry = readdir(dir);
        if (is_normal_entry(entry->d_name)) {
            ++line;
        }
    }

    while (line < start_line + str_count && (entry = readdir(dir))) {
        print_line(entry, &line);
    }
    closedir(dir);
}

static size_t get_entries_count(void) {
    DIR *dir = opendir(".");
    struct dirent *entry;
    size_t ans = 0;
    while (entry = readdir(dir)) {
        if (is_normal_entry(entry->d_name)) {
            ++ans;
        }
    }
    ++ans;
    closedir(dir);
    return ans;
}

void init_program(void) {
    screen = initscr();
    if (!screen) {
        clear_data();
        perror("Couldn't open init screen");
        exit(EXIT_FAILURE);
    }
    noecho();
    keypad(screen, true);
    curs_set(0);
    init_colors();
    getmaxyx(screen, str_count, col_count);
    header_str = calloc(col_count + 1, sizeof(*header_str));
    memset(header_str, ' ', col_count * sizeof(*header_str));
    strcpy(header_str, "Name");
    header_str[4] = ' ';
    strcpy(header_str + col_count - 4, "Size");
    print_dirs();
}

void move_arrow_down(void) {
    if (current_line + 1 != get_entries_count()) {
        ++current_line;
        if (start_line + str_count - 1 <= current_line) {
            ++start_line;
        }
    }
}

void move_arrow_up(void) {
    if (current_line != 0) {
        --current_line;
        if (current_line < start_line && start_line > 1) {
            --start_line;
        }
    }
}

void go_to_directory(void) {
    if (current_line == 0) {
        return;
    }
    FILE *file = fopen("log.txt", "w+");
    struct dirent *entry = get_cur_entry();
    if (entry->d_type == DT_DIR) {
        fprintf(file, "%s\n", entry->d_name);
        if (access(entry->d_name, R_OK) != 0) {
            close_cur_dir();
            return;
        }
        if (chdir(entry->d_name) != 0) {
            close_cur_dir();
            return;
        }
        start_line = 1;
        current_line = 0;
        fprintf(file, "here\n");
    }
    fclose(file);
    close_cur_dir();
}

void remove_entry(void) {
    if (current_line == 0) {
        return;
    }
    struct dirent *entry = get_cur_entry();

    if (strcmp(entry->d_name, "..") != 0) {
        remove(entry->d_name);
    }
    close_cur_dir();
}

void copy_file(bool need_extract) {
    if (current_line == 0) {
        return;
    }
    struct dirent *entry = get_cur_entry();
    if (entry->d_type == DT_REG) {
        free(data_file_path);
        free(data_file_name);
        data_file_path = realpath(entry->d_name, NULL);
        data_file_name = calloc(strlen(entry->d_name) + 1, sizeof(*data_file_name));
        strcpy(data_file_name, entry->d_name);
        is_extract = need_extract;
    }
    close_cur_dir();
}

void paste_file(void) {
    if (!data_file_path || !data_file_name) {
        return;
    }
    FILE *file = fopen(data_file_path, "r");
    if (!file) {
        return;
    }
    if (access(data_file_name, F_OK) == 0) {
        fclose(file);
        return;
    }
    FILE *new_file = fopen(data_file_name, "w+");
    if (!new_file) {
        fclose(new_file);
        fclose(file);
        return;
    }
    int c;
    while ((c = fgetc(file)) != EOF) {
        fputc(c, new_file);
    } 
    fclose(new_file);
    fclose(file);
    if (is_extract) {
        remove(data_file_path);
    }
}

void switch_hidden_files(void) {
    show_hidden_files = !show_hidden_files;
    start_line = 1;
    current_line = 0;
}