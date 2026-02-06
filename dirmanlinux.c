#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ncurses.h>
#include <unistd.h>
#include <sys/stat.h>

#define MAX_OPTIONS 100
#define MAX_PATH 4096

typedef struct {
    char name[256];
    int is_dir;
} FileEntry;

// ASCII cizgi karakterleri
#define CORNER_TL "+"
#define CORNER_TR "+"
#define CORNER_BL "+"
#define CORNER_BR "+"
#define H_LINE "-"
#define V_LINE "|"

void draw_box(int start_y, int start_x, int height, int width) {
    // Ust cizgi
    mvprintw(start_y, start_x, CORNER_TL);
    for (int i = 0; i < width - 2; i++) addstr(H_LINE);
    addstr(CORNER_TR);
    
    // Yan cizgiler
    for (int i = 1; i < height - 1; i++) {
        mvprintw(start_y + i, start_x, V_LINE);
        mvprintw(start_y + i, start_x + width - 1, V_LINE);
    }
    
    // Alt cizgi
    mvprintw(start_y + height - 1, start_x, CORNER_BL);
    for (int i = 0; i < width - 2; i++) addstr(H_LINE);
    addstr(CORNER_BR);
}

void draw_menu(int highlight, FileEntry options[], int n_options, const char *current_dir) {
    clear();
    
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    // Baslik kutusu
    draw_box(0, 0, 3, max_x);
    attron(A_BOLD);
    mvprintw(1, 2, "[ %s ]", current_dir);
    attroff(A_BOLD);
    
    // Icerik kutusu
    int content_height = max_y - 6;
    draw_box(3, 0, content_height, max_x);
    
    // Secenekleri listele
    int start_row = 4;
    for (int i = 0; i < n_options && i < content_height - 2; i++) {
        int row = start_row + i;
        
        if (i == highlight) {
            attron(A_REVERSE);
            mvprintw(row, 1, ">");
        } else {
            mvprintw(row, 1, " ");
        }
        
        // Klasor/Dosya gostergesi
        const char *icon = options[i].is_dir ? "[DIR]" : "[FIL]";
        mvprintw(row, 3, "%s %-50s", icon, options[i].name);
        
        if (i == highlight) {
            attroff(A_REVERSE);
        }
    }
    
    // Alt bilgi kutusu
    draw_box(max_y - 3, 0, 3, max_x);
    mvprintw(max_y - 2, 2, "UP/DOWN:Move | ENTER:Open | -:Up | q:Quit");
    
    refresh();
}

int list_directory(const char *dir_name, FileEntry entries[]) {
    DIR *d = opendir(dir_name);
    if (!d) return -1;

    struct dirent *dir;
    int count = 0;

    while ((dir = readdir(d)) != NULL && count < MAX_OPTIONS) {
        if (strcmp(dir->d_name, ".") == 0) continue;
        
        strncpy(entries[count].name, dir->d_name, 255);
        entries[count].name[255] = '\0';
        
        struct stat st;
        char full_path[MAX_PATH];
        snprintf(full_path, MAX_PATH, "%s/%s", dir_name, dir->d_name);
        
        entries[count].is_dir = (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode));
        count++;
    }
    
    closedir(d);
    return count;
}

void file_manager() {
    FileEntry entries[MAX_OPTIONS];
    int highlight = 0;
    int n_options;
    char current_dir[MAX_PATH];

    getcwd(current_dir, sizeof(current_dir));

    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_CYAN, COLOR_BLACK);
    }

    while (1) {
        n_options = list_directory(current_dir, entries);
        
        if (n_options < 0) {
            mvprintw(0, 0, "Dizin okunamadi: %s", current_dir);
            refresh();
            getch();
            break;
        }
        
        if (highlight >= n_options) highlight = n_options - 1;
        if (highlight < 0) highlight = 0;
        
        draw_menu(highlight, entries, n_options, current_dir);

        int ch = getch();

        switch (ch) {
            case KEY_UP:
                if (highlight > 0) highlight--;
                break;
                
            case KEY_DOWN:
                if (highlight < n_options - 1) highlight++;
                break;
                
            case KEY_LEFT:
            case '-':
                if (chdir("..") == 0) {
                    getcwd(current_dir, sizeof(current_dir));
                    highlight = 0;
                }
                break;
                
            case KEY_RIGHT:
            case 10: // Enter
                if (entries[highlight].is_dir) {
                    if (chdir(entries[highlight].name) == 0) {
                        getcwd(current_dir, sizeof(current_dir));
                        highlight = 0;
                    }
                }
                break;
                
            case 'q':
            case 27:
                goto cleanup;
        }
    }

cleanup:
    endwin();
}

int main() {
    file_manager();
    return 0;
}
