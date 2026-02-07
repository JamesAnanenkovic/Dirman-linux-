#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ncurses.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <errno.h>

#define MAX_OPTIONS 100
#define MAX_PATH 4096
#define CLIPBOARD_SIZE 10

typedef struct {
    char name[256];
    int is_dir;
    off_t size;
} FileEntry;

// Clipboard yapısı
typedef struct {
    char path[MAX_PATH];
    char name[256];
    int is_dir;
    int is_cut;  // 1: move, 0: copy
    int active;
} ClipboardItem;

ClipboardItem clipboard[CLIPBOARD_SIZE];
int clipboard_count = 0;

typedef struct {
    char name[20];
    int border_color;
    int title_color;
    int dir_color;
    int file_color;
    int highlight_color;
    int text_color;
    int status_color;
    int danger_color;
    int success_color;
    int info_color;
} ColorScheme;

ColorScheme schemes[] = {
    {"Default", COLOR_WHITE, COLOR_CYAN, COLOR_BLUE, COLOR_BLUE, COLOR_CYAN, COLOR_WHITE, COLOR_WHITE, COLOR_RED, COLOR_GREEN, COLOR_YELLOW},
    {"Ocean", COLOR_CYAN, COLOR_BLUE, COLOR_BLUE, COLOR_BLUE, COLOR_BLUE, COLOR_CYAN, COLOR_CYAN, COLOR_RED, COLOR_GREEN, COLOR_YELLOW},
    {"Forest", COLOR_GREEN, COLOR_GREEN, COLOR_GREEN, COLOR_GREEN, COLOR_GREEN, COLOR_WHITE, COLOR_GREEN, COLOR_RED, COLOR_YELLOW, COLOR_CYAN},
    {"Sunset", COLOR_RED, COLOR_YELLOW, COLOR_YELLOW, COLOR_YELLOW, COLOR_RED, COLOR_YELLOW, COLOR_RED, COLOR_MAGENTA, COLOR_GREEN, COLOR_CYAN},
    {"Matrix", COLOR_GREEN, COLOR_GREEN, COLOR_GREEN, COLOR_GREEN, COLOR_GREEN, COLOR_GREEN, COLOR_GREEN, COLOR_RED, COLOR_YELLOW, COLOR_WHITE},
    {"Mono", COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE},
    {"Gold", COLOR_YELLOW, COLOR_YELLOW, COLOR_YELLOW, COLOR_YELLOW, COLOR_YELLOW, COLOR_YELLOW, COLOR_YELLOW, COLOR_RED, COLOR_GREEN, COLOR_CYAN},
    {"Purple", COLOR_MAGENTA, COLOR_MAGENTA, COLOR_GREEN, COLOR_GREEN, COLOR_MAGENTA, COLOR_MAGENTA, COLOR_MAGENTA, COLOR_RED, COLOR_GREEN, COLOR_CYAN},
};
int num_schemes = 8;
int current_scheme = 0;
int color_enabled = 1;

#define CORNER_TL "+"
#define CORNER_TR "+"
#define CORNER_BL "+"
#define CORNER_BR "+"
#define H_LINE "-"
#define V_LINE "|"

void init_colors() {
    if (!has_colors() || !color_enabled) return;
    start_color();
    use_default_colors();
    init_pair(1, schemes[current_scheme].border_color, -1);
    init_pair(2, schemes[current_scheme].title_color, -1);
    init_pair(3, schemes[current_scheme].dir_color, -1);
    init_pair(4, schemes[current_scheme].file_color, -1);
    init_pair(5, schemes[current_scheme].highlight_color, -1);
    init_pair(6, schemes[current_scheme].text_color, -1);
    init_pair(7, schemes[current_scheme].status_color, -1);
    init_pair(8, COLOR_BLACK, schemes[current_scheme].highlight_color);
    init_pair(9, schemes[current_scheme].danger_color, -1);
    init_pair(10, schemes[current_scheme].success_color, -1);
    init_pair(11, schemes[current_scheme].info_color, -1);
    init_pair(12, COLOR_MAGENTA, -1); // Clipboard indicator
}

void draw_box_colored(int start_y, int start_x, int height, int width, int color_pair) {
    if (color_enabled) attron(COLOR_PAIR(color_pair));
    mvprintw(start_y, start_x, CORNER_TL);
    for (int i = 0; i < width - 2; i++) addstr(H_LINE);
    addstr(CORNER_TR);
    for (int i = 1; i < height - 1; i++) {
        mvprintw(start_y + i, start_x, V_LINE);
        mvprintw(start_y + i, start_x + width - 1, V_LINE);
    }
    mvprintw(start_y + height - 1, start_x, CORNER_BL);
    for (int i = 0; i < width - 2; i++) addstr(H_LINE);
    addstr(CORNER_BR);
    if (color_enabled) attroff(COLOR_PAIR(color_pair));
}

// BOYUT FORMATLAMA
void format_size(off_t size, char *buf, size_t buflen) {
    const char *units[] = {"B", "K", "M", "G", "T"};
    int unit = 0;
    double dsize = (double)size;
    
    while (dsize >= 1024.0 && unit < 4) {
        dsize /= 1024.0;
        unit++;
    }
    
    if (unit == 0) {
        snprintf(buf, buflen, "%ld%s", size, units[unit]);
    } else {
        snprintf(buf, buflen, "%.1f%s", dsize, units[unit]);
    }
}

int confirm_dialog(const char *message, const char *item_name) {
    int ch, selected = 1;
    while (1) {
        clear();
        int max_y, max_x;
        getmaxyx(stdscr, max_y, max_x);
        int box_width = 60, box_height = 8;
        int start_x = (max_x - box_width) / 2, start_y = (max_y - box_height) / 2;
        
        draw_box_colored(start_y, start_x, box_height, box_width, 9);
        if (color_enabled) attron(COLOR_PAIR(9) | A_BOLD);
        mvprintw(start_y + 2, start_x + 3, "%s", message);
        if (color_enabled) attroff(COLOR_PAIR(9) | A_BOLD);
        if (color_enabled) attron(COLOR_PAIR(11) | A_BOLD);
        mvprintw(start_y + 3, start_x + 5, "%s", item_name);
        if (color_enabled) attroff(COLOR_PAIR(11) | A_BOLD);
        
        int btn_y = start_y + 5;
        if (selected == 0) {
            if (color_enabled) attron(COLOR_PAIR(8));
            mvprintw(btn_y, start_x + 15, " [ HAYIR ] ");
            if (color_enabled) attroff(COLOR_PAIR(8));
            if (color_enabled) attron(COLOR_PAIR(6));
            mvprintw(btn_y, start_x + 35, "   EVET   ");
            if (color_enabled) attroff(COLOR_PAIR(6));
        } else {
            if (color_enabled) attron(COLOR_PAIR(6));
            mvprintw(btn_y, start_x + 15, "   HAYIR   ");
            if (color_enabled) attroff(COLOR_PAIR(6));
            if (color_enabled) attron(COLOR_PAIR(8));
            mvprintw(btn_y, start_x + 35, " [ EVET ] ");
            if (color_enabled) attroff(COLOR_PAIR(8));
        }
        refresh();
        ch = getch();
        switch (ch) {
            case KEY_LEFT: case KEY_RIGHT: selected = !selected; break;
            case 10: return selected;
            case 27: case 'n': case 'N': return 0;
            case 'y': case 'Y': return 1;
        }
    }
}

int input_dialog(const char *title, char *buffer, int max_len, int is_dir) {
    int ch, pos = 0;
    buffer[0] = '\0';
    while (1) {
        clear();
        int max_y, max_x;
        getmaxyx(stdscr, max_y, max_x);
        int box_width = 60, box_height = 7;
        int start_x = (max_x - box_width) / 2, start_y = (max_y - box_height) / 2;
        
        draw_box_colored(start_y, start_x, box_height, box_width, is_dir ? 3 : 4);
        if (color_enabled) attron(COLOR_PAIR(2) | A_BOLD);
        mvprintw(start_y + 1, start_x + 3, "%s", title);
        if (color_enabled) attroff(COLOR_PAIR(2) | A_BOLD);
        
        if (color_enabled) attron(COLOR_PAIR(6));
        mvprintw(start_y + 3, start_x + 3, "[");
        for (int i = 0; i < box_width - 8; i++) addstr("_");
        addstr("]");
        mvprintw(start_y + 3, start_x + 4, "%s", buffer);
        if (color_enabled) attroff(COLOR_PAIR(6));
        
        if (color_enabled) attron(COLOR_PAIR(11));
        mvprintw(start_y + 5, start_x + 3, "Enter:Onay | ESC:Iptal");
        if (color_enabled) attroff(COLOR_PAIR(11));
        
        refresh();
        ch = getch();
        switch (ch) {
            case 27: return 0;
            case 10: if (pos > 0) return 1; break;
            case KEY_BACKSPACE: case 127: case '\b':
                if (pos > 0) buffer[--pos] = '\0';
                break;
            default:
                if (pos < max_len - 1 && ch >= 32 && ch < 127) {
                    buffer[pos++] = ch;
                    buffer[pos] = '\0';
                }
                break;
        }
    }
}

void info_dialog(const char *message, int is_success) {
    clear();
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    int box_width = 50, box_height = 5;
    int start_x = (max_x - box_width) / 2, start_y = (max_y - box_height) / 2;
    
    draw_box_colored(start_y, start_x, box_height, box_width, is_success ? 10 : 9);
    if (color_enabled) attron(COLOR_PAIR(is_success ? 10 : 9) | A_BOLD);
    mvprintw(start_y + 2, start_x + (box_width - strlen(message)) / 2, "%s", message);
    if (color_enabled) attroff(COLOR_PAIR(is_success ? 10 : 9) | A_BOLD);
    refresh();
    getch();
}

// CLIPBOARD GÖSTERİMİ
void draw_clipboard_status() {
    if (clipboard_count == 0) return;
    
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    char msg[100];
    snprintf(msg, sizeof(msg), "[CLIPBOARD: %d item%s (%s)]", 
        clipboard_count, 
        clipboard_count > 1 ? "s" : "",
        clipboard[0].is_cut ? "MOVE" : "COPY");
    
    if (color_enabled) attron(COLOR_PAIR(12) | A_BOLD);
    mvprintw(2, max_x - strlen(msg) - 3, "%s", msg);
    if (color_enabled) attroff(COLOR_PAIR(12) | A_BOLD);
}

void draw_menu(int highlight, FileEntry options[], int n_options, const char *current_dir) {
    clear();
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    draw_box_colored(0, 0, 3, max_x, 1);
    if (color_enabled) attron(COLOR_PAIR(2) | A_BOLD);
    mvprintw(1, 2, "[ %s ]", current_dir);
    if (color_enabled) attroff(COLOR_PAIR(2) | A_BOLD);
    
    draw_clipboard_status();
    
    int content_height = max_y - 6;
    draw_box_colored(3, 0, content_height, max_x, 1);
    
    int start_row = 4;
    int name_width = max_x - 20; // Boyut için yer bırak
    
    for (int i = 0; i < n_options && i < content_height - 2; i++) {
        int row = start_row + i;
        char size_str[10];
        format_size(options[i].size, size_str, sizeof(size_str));
        
        // Clipboard'ta mı kontrol et
        int in_clipboard = 0;
        for (int c = 0; c < clipboard_count; c++) {
            if (strcmp(clipboard[c].name, options[i].name) == 0) {
                in_clipboard = 1;
                break;
            }
        }
        
        if (i == highlight) {
            if (color_enabled) attron(COLOR_PAIR(8));
            mvprintw(row, 1, "%s>", in_clipboard ? "*" : " ");
            mvprintw(row, 3, "%-.*s", name_width - 3, "");
            if (color_enabled) attroff(COLOR_PAIR(8));
            
            if (color_enabled) attron(COLOR_PAIR(8));
            mvprintw(row, 3, "%s %s", options[i].is_dir ? "[DIR]" : "[FIL]", options[i].name);
            // Boyut
            mvprintw(row, max_x - 10, "%8s", size_str);
            if (color_enabled) attroff(COLOR_PAIR(8));
        } else {
            mvprintw(row, 1, "%s ", in_clipboard ? "*" : " ");
            if (color_enabled) attron(COLOR_PAIR(options[i].is_dir ? 3 : 4));
            mvprintw(row, 3, "%s %s", options[i].is_dir ? "[DIR]" : "[FIL]", options[i].name);
            // Boyut sağda
            if (color_enabled) attron(COLOR_PAIR(11));
            mvprintw(row, max_x - 10, "%8s", size_str);
            if (color_enabled) attroff(COLOR_PAIR(11));
            if (color_enabled) attroff(COLOR_PAIR(options[i].is_dir ? 3 : 4));
        }
    }
    
    draw_box_colored(max_y - 3, 0, 3, max_x, 1);
    if (color_enabled) attron(COLOR_PAIR(7));
    mvprintw(max_y - 2, 2, "ENTER:Open | c:Copy | m:Move | p:Paste | R:Del | n:NewF | N:NewD | -:Up | ESC:Menu | q:Quit");
    if (color_enabled) attroff(COLOR_PAIR(7));
    
    refresh();
}

// DOSYA KOPYALAMA (sendfile ile hızlı)
int copy_file(const char *src, const char *dst) {
    int fd_src = open(src, O_RDONLY);
    if (fd_src < 0) return -1;
    
    struct stat st;
    fstat(fd_src, &st);
    
    int fd_dst = open(dst, O_WRONLY | O_CREAT | O_TRUNC, st.st_mode);
    if (fd_dst < 0) {
        close(fd_src);
        return -1;
    }
    
    off_t offset = 0;
    ssize_t sent = sendfile(fd_dst, fd_src, &offset, st.st_size);
    
    close(fd_src);
    close(fd_dst);
    
    return (sent == st.st_size) ? 0 : -1;
}

// KLASÖR KOPYALAMA (recursive)
int copy_directory(const char *src, const char *dst) {
    struct stat st;
    if (stat(src, &st) < 0) return -1;
    
    if (mkdir(dst, st.st_mode) < 0 && errno != EEXIST) return -1;
    
    DIR *d = opendir(src);
    if (!d) return -1;
    
    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) {
        if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) continue;
        
        char src_path[MAX_PATH], dst_path[MAX_PATH];
        snprintf(src_path, sizeof(src_path), "%s/%s", src, dir->d_name);
        snprintf(dst_path, sizeof(dst_path), "%s/%s", dst, dir->d_name);
        
        struct stat entry_st;
        if (stat(src_path, &entry_st) == 0) {
            if (S_ISDIR(entry_st.st_mode)) {
                if (copy_directory(src_path, dst_path) < 0) {
                    closedir(d);
                    return -1;
                }
            } else {
                if (copy_file(src_path, dst_path) < 0) {
                    closedir(d);
                    return -1;
                }
            }
        }
    }
    closedir(d);
    return 0;
}

void settings_menu() {
    char *options[] = {
        "Color: ON/OFF",
        "Scheme: Default", "Scheme: Ocean", "Scheme: Forest", "Scheme: Sunset",
        "Scheme: Matrix", "Scheme: Mono", "Scheme: Gold", "Scheme: Purple",
        "Clear Clipboard",
        "Back"
    };
    int n_options = 11;
    int highlight = 0;
    int ch;
    
    while (1) {
        clear();
        int max_y, max_x;
        getmaxyx(stdscr, max_y, max_x);
        
        int box_width = 50;
        int box_height = n_options + 4;
        int start_x = (max_x - box_width) / 2;
        int start_y = (max_y - box_height) / 2;
        
        draw_box_colored(start_y, start_x, box_height, box_width, 1);
        if (color_enabled) attron(COLOR_PAIR(2) | A_BOLD);
        mvprintw(start_y + 1, start_x + 18, "SETTINGS");
        if (color_enabled) attroff(COLOR_PAIR(2) | A_BOLD);
        
        // Clipboard bilgisi
        if (clipboard_count > 0) {
            if (color_enabled) attron(COLOR_PAIR(12));
            mvprintw(start_y + 2, start_x + 10, "Clipboard: %d items", clipboard_count);
            if (color_enabled) attroff(COLOR_PAIR(12));
        }
        
        for (int i = 0; i < n_options; i++) {
            int row = start_y + 4 + i;
            if (i == highlight) {
                if (color_enabled) attron(COLOR_PAIR(8));
                mvprintw(row, start_x + 2, " > %-45s", "");
                mvprintw(row, start_x + 4, "%s", options[i]);
                if (color_enabled) attroff(COLOR_PAIR(8));
            } else {
                if (i == 0) {
                    const char *status = color_enabled ? "ON " : "OFF";
                    if (color_enabled) attron(COLOR_PAIR(6));
                    mvprintw(row, start_x + 3, "[ %s ] %s", status, options[i]);
                    if (color_enabled) attroff(COLOR_PAIR(6));
                } else if (i >= 1 && i <= 8) {
                    int scheme_idx = i - 1;
                    const char *marker = (current_scheme == scheme_idx) ? ">>" : "  ";
                    if (color_enabled) attron(COLOR_PAIR(schemes[scheme_idx].title_color));
                    mvprintw(row, start_x + 3, "%s %s", marker, options[i]);
                    if (color_enabled) attroff(COLOR_PAIR(schemes[scheme_idx].title_color));
                } else if (i == 9) {
                    if (color_enabled) attron(COLOR_PAIR(9));
                    mvprintw(row, start_x + 3, "   %s", options[i]);
                    if (color_enabled) attroff(COLOR_PAIR(9));
                } else {
                    if (color_enabled) attron(COLOR_PAIR(6));
                    mvprintw(row, start_x + 3, "   %s", options[i]);
                    if (color_enabled) attroff(COLOR_PAIR(6));
                }
            }
        }
        
        refresh();
        ch = getch();
        
        switch (ch) {
            case KEY_UP: if (highlight > 0) highlight--; break;
            case KEY_DOWN: if (highlight < n_options - 1) highlight++; break;
            case 10:
            case ' ':
                switch (highlight) {
                    case 0: color_enabled = !color_enabled; break;
                    case 1: current_scheme = 0; color_enabled = 1; break;
                    case 2: current_scheme = 1; color_enabled = 1; break;
                    case 3: current_scheme = 2; color_enabled = 1; break;
                    case 4: current_scheme = 3; color_enabled = 1; break;
                    case 5: current_scheme = 4; color_enabled = 1; break;
                    case 6: current_scheme = 5; color_enabled = 1; break;
                    case 7: current_scheme = 6; color_enabled = 1; break;
                    case 8: current_scheme = 7; color_enabled = 1; break;
                    case 9: clipboard_count = 0; break;
                    case 10: init_colors(); return;
                }
                init_colors();
                break;
            case 27:
            case 'q':
                init_colors();
                return;
        }
    }
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
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_name, dir->d_name);
        if (stat(full_path, &st) == 0) {
            entries[count].is_dir = S_ISDIR(st.st_mode);
            entries[count].size = st.st_size;
        } else {
            entries[count].is_dir = 0;
            entries[count].size = 0;
        }
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
    char input_buffer[256];

    getcwd(current_dir, sizeof(current_dir));

    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    init_colors();

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
            case 10:
                if (entries[highlight].is_dir) {
                    if (chdir(entries[highlight].name) == 0) {
                        getcwd(current_dir, sizeof(current_dir));
                        highlight = 0;
                    }
                }
                break;
            case 'c': // COPY
            case 'C': {
                if (n_options == 0) break;
                if (clipboard_count >= CLIPBOARD_SIZE) {
                    info_dialog("Clipboard dolu!", 0);
                    break;
                }
                // Aynı item var mı kontrol et
                int exists = 0;
                for (int i = 0; i < clipboard_count; i++) {
                    if (strcmp(clipboard[i].name, entries[highlight].name) == 0) {
                        exists = 1;
                        break;
                    }
                }
                if (!exists) {
                    snprintf(clipboard[clipboard_count].path, MAX_PATH, "%s/%s", current_dir, entries[highlight].name);
                    strncpy(clipboard[clipboard_count].name, entries[highlight].name, 256);
                    clipboard[clipboard_count].is_dir = entries[highlight].is_dir;
                    clipboard[clipboard_count].is_cut = 0;
                    clipboard[clipboard_count].active = 1;
                    clipboard_count++;
                    char msg[300];
                    snprintf(msg, sizeof(msg), "'%s' kopyalandi", entries[highlight].name);
                    info_dialog(msg, 1);
                }
                break;
            }
            case 'm': // MOVE (CUT)
            case 'M': {
                if (n_options == 0) break;
                if (clipboard_count >= CLIPBOARD_SIZE) {
                    info_dialog("Clipboard dolu!", 0);
                    break;
                }
                // Önceki cut'ları temizle (sadece bir move olabilir)
                for (int i = 0; i < clipboard_count; i++) {
                    if (clipboard[i].is_cut) {
                        // Shift et
                        for (int j = i; j < clipboard_count - 1; j++) {
                            clipboard[j] = clipboard[j + 1];
                        }
                        clipboard_count--;
                        i--;
                    }
                }
                snprintf(clipboard[clipboard_count].path, MAX_PATH, "%s/%s", current_dir, entries[highlight].name);
                strncpy(clipboard[clipboard_count].name, entries[highlight].name, 256);
                clipboard[clipboard_count].is_dir = entries[highlight].is_dir;
                clipboard[clipboard_count].is_cut = 1;
                clipboard[clipboard_count].active = 1;
                clipboard_count++;
                char msg[300];
                snprintf(msg, sizeof(msg), "'%s' tasima icin hazir", entries[highlight].name);
                info_dialog(msg, 1);
                break;
            }
            case 'p': // PASTE
            case 'P': {
                if (clipboard_count == 0) {
                    info_dialog("Clipboard bos!", 0);
                    break;
                }
                int success_count = 0;
                int fail_count = 0;
                
                for (int i = 0; i < clipboard_count; i++) {
                    char dest_path[MAX_PATH];
                    snprintf(dest_path, sizeof(dest_path), "%s/%s", current_dir, clipboard[i].name);
                    
                    // Aynı dizine yapıştırma kontrolü
                    if (strcmp(clipboard[i].path, dest_path) == 0) continue;
                    
                    // Hedef var mı kontrol et
                    struct stat st;
                    if (stat(dest_path, &st) == 0) {
                        char msg[512];
                        snprintf(msg, sizeof(msg), "'%s' zaten var, uzerine yazilsin mi?", clipboard[i].name);
                        if (!confirm_dialog(msg, "UYARI: Var olan dosya silinecek!")) {
                            continue;
                        }
                    }
                    
                    int result;
                    if (clipboard[i].is_dir) {
                        if (clipboard[i].is_cut) {
                            result = rename(clipboard[i].path, dest_path);
                        } else {
                            result = copy_directory(clipboard[i].path, dest_path);
                        }
                    } else {
                        if (clipboard[i].is_cut) {
                            result = rename(clipboard[i].path, dest_path);
                        } else {
                            result = copy_file(clipboard[i].path, dest_path);
                        }
                    }
                    
                    if (result == 0) {
                        success_count++;
                    } else {
                        fail_count++;
                    }
                }
                
                // Move yapıldıysa clipboard'u temizle
                int had_cut = 0;
                for (int i = 0; i < clipboard_count; i++) {
                    if (clipboard[i].is_cut) had_cut = 1;
                }
                if (had_cut) clipboard_count = 0;
                
                char msg[256];
                if (fail_count == 0) {
                    snprintf(msg, sizeof(msg), "%d oge yapistirildi", success_count);
                    info_dialog(msg, 1);
                } else {
                    snprintf(msg, sizeof(msg), "%d basari, %d basarisiz", success_count, fail_count);
                    info_dialog(msg, 0);
                }
                break;
            }
            case 'r':
            case 'R': {
                if (n_options == 0) break;
                char msg[512];
                snprintf(msg, sizeof(msg), "Silmek istediginize emin misiniz?");
                char item[300];
                snprintf(item, sizeof(item), "%s %s", 
                    entries[highlight].is_dir ? "[DIR]" : "[FIL]",
                    entries[highlight].name);
                
                if (confirm_dialog(msg, item)) {
                    char full_path[MAX_PATH];
                    snprintf(full_path, sizeof(full_path), "%s/%s", 
                        current_dir, entries[highlight].name);
                    
                    int success;
                    if (entries[highlight].is_dir) {
                        success = rmdir(full_path);
                    } else {
                        success = unlink(full_path);
                    }
                    
                    if (success == 0) {
                        info_dialog("Basariyla silindi!", 1);
                    } else {
                        info_dialog("Silme basarisiz! (Klasor bos olmayabilir)", 0);
                    }
                }
                break;
            }
            case 'n': {
                if (input_dialog("Yeni Dosya Adi:", input_buffer, sizeof(input_buffer), 0)) {
                    char full_path[MAX_PATH];
                    snprintf(full_path, sizeof(full_path), "%s/%s", current_dir, input_buffer);
                    FILE *f = fopen(full_path, "w");
                    if (f) {
                        fclose(f);
                        info_dialog("Dosya olusturuldu!", 1);
                    } else {
                        info_dialog("Dosya olusturulamadi!", 0);
                    }
                }
                break;
            }
            case 'N': {
                if (input_dialog("Yeni Klasor Adi:", input_buffer, sizeof(input_buffer), 1)) {
                    char full_path[MAX_PATH];
                    snprintf(full_path, sizeof(full_path), "%s/%s", current_dir, input_buffer);
                    if (mkdir(full_path, 0755) == 0) {
                        info_dialog("Klasor olusturuldu!", 1);
                    } else {
                        info_dialog("Klasor olusturulamadi!", 0);
                    }
                }
                break;
            }
            case 27:
                settings_menu();
                clear();
                refresh();
                break;
            case 'q':
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
