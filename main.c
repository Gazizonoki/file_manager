#include <stdlib.h>

#include "events.h"

int main(void) {
    init_program();
    while (true) {
        int c = getch();
        if (c == 'q') {
            break;
        } else if (c == KEY_UP) {
            move_arrow_up();
        } else if (c == KEY_DOWN) {
            move_arrow_down();
        } else if (c == '\n') {
            go_to_directory();
        } else if (c == 'd') {
            remove_entry();
        } else if (c == 'c') {
            copy_file(false);
        } else if (c == 'x') {
            copy_file(true);
        } else if (c == 'v') {
            paste_file();
        } else if (c == 'h') {
            switch_hidden_files();
        } else {
            continue;
        }
        print_dirs();
    }
    clear_data();
    return EXIT_SUCCESS;
}