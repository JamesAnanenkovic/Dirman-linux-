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
#include <ctype.h>
#include <stdarg.h>

#define MAX_OPTIONS 100000
#define MAX_PATH 4096
#define CLIPBOARD_SIZE 1000
#define PAGE_SIZE 100
#define MAX_FILTER_LEN 256

// ERROR HANDLING
#define CHECK_NULL(ptr, msg) do { if (!(ptr)) { status_error(msg); return 0; } } while(0)
#define CHECK_NEG(val, msg) do { if ((val) < 0) { status_error(msg); return 0; } } while(0)

// ACTIONS
typedef enum {
    ACTION_NONE, ACTION_UP, ACTION_DOWN, ACTION_LEFT, ACTION_RIGHT,
    ACTION_ENTER, ACTION_QUIT, ACTION_MENU,
    ACTION_COPY, ACTION_MOVE, ACTION_PASTE, ACTION_DELETE,
    ACTION_NEW_FILE, ACTION_NEW_DIR,
    ACTION_SELECT, ACTION_SELECT_ALL, ACTION_SELECT_CLEAR,
    ACTION_FILTER, ACTION_CLEAR_FILTER,
    ACTION_PAGE_UP, ACTION_PAGE_DOWN,
    ACTION_GOTO_TOP, ACTION_GOTO_BOTTOM
} Action;

typedef struct {
    int key;
    Action action;
} KeyMap;

typedef struct {
    char name[256];
    int is_dir;
    off_t size;
    int selected;
    int visible;
} FileEntry;

typedef struct {
    char path[MAX_PATH];
    char name[256];
    int is_dir;
    int is_cut;
    int active;
} ClipboardItem;

typedef struct {
    FileEntry *entries;
    int n_options;
    int highlight;
    int page_start;
    int page_count;
    char current_dir[MAX_PATH];
    char filter[MAX_FILTER_LEN];
    int filter_active;
    int select_count;
} AppState;

typedef struct {
    char name[20];
    int border, title, dir, file, highlight, text, status, danger, success, info, warning;
} ColorScheme;

// BIOS MENU TYPES
typedef enum {
    TAB_COLORS,
    TAB_VIEW,
    TAB_KEYS,
    TAB_ABOUT,
    NUM_TABS
} MenuTab;

const char *tab_names[] = {"Colors", "View", "Keys", "About"};

ColorScheme schemes[] = {
    {"Default", COLOR_WHITE, COLOR_CYAN, COLOR_BLUE, COLOR_BLUE, COLOR_CYAN, COLOR_WHITE, COLOR_WHITE, COLOR_RED, COLOR_GREEN, COLOR_YELLOW, COLOR_MAGENTA},
    {"Ocean", COLOR_CYAN, COLOR_BLUE, COLOR_BLUE, COLOR_BLUE, COLOR_BLUE, COLOR_CYAN, COLOR_CYAN, COLOR_RED, COLOR_GREEN, COLOR_YELLOW, COLOR_MAGENTA},
    {"Forest", COLOR_GREEN, COLOR_GREEN, COLOR_GREEN, COLOR_GREEN, COLOR_GREEN, COLOR_WHITE, COLOR_GREEN, COLOR_RED, COLOR_YELLOW, COLOR_CYAN, COLOR_MAGENTA},
    {"Sunset", COLOR_RED, COLOR_YELLOW, COLOR_YELLOW, COLOR_YELLOW, COLOR_RED, COLOR_YELLOW, COLOR_RED, COLOR_MAGENTA, COLOR_GREEN, COLOR_CYAN, COLOR_WHITE},
    {"Matrix", COLOR_GREEN, COLOR_GREEN, COLOR_GREEN, COLOR_GREEN, COLOR_GREEN, COLOR_GREEN, COLOR_GREEN, COLOR_RED, COLOR_YELLOW, COLOR_WHITE, COLOR_CYAN},
    {"Mono", COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE},
    {"Gold", COLOR_YELLOW, COLOR_YELLOW, COLOR_YELLOW, COLOR_YELLOW, COLOR_YELLOW, COLOR_YELLOW, COLOR_YELLOW, COLOR_RED, COLOR_GREEN, COLOR_CYAN, COLOR_MAGENTA},
    {"Purple", COLOR_MAGENTA, COLOR_MAGENTA, COLOR_MAGENTA, COLOR_MAGENTA, COLOR_MAGENTA, COLOR_MAGENTA, COLOR_MAGENTA, COLOR_RED, COLOR_GREEN, COLOR_CYAN, COLOR_YELLOW},
};
int num_schemes = 8, current_scheme = 0, color_enabled = 1;

ClipboardItem clipboard[CLIPBOARD_SIZE];
int clipboard_count = 0;
AppState app = {0};
char status_msg[256] = {0};
int status_is_error = 0;

// ASCII BOX CHARACTERS
#define CORNER_TL "+"
#define CORNER_TR "+"
#define CORNER_BL "+"
#define CORNER_BR "+"
#define H_LINE "-"
#define V_LINE "|"

KeyMap keymap[] = {
    {KEY_UP, ACTION_UP}, {'k', ACTION_UP},
    {KEY_DOWN, ACTION_DOWN}, {'j', ACTION_DOWN},
    {KEY_LEFT, ACTION_LEFT}, {'h', ACTION_LEFT}, {'-', ACTION_LEFT},
    {KEY_RIGHT, ACTION_RIGHT}, {'l', ACTION_RIGHT},
    {10, ACTION_ENTER},
    {'q', ACTION_QUIT},
    {27, ACTION_MENU},
    {'c', ACTION_COPY}, {'C', ACTION_COPY},
    {'m', ACTION_MOVE}, {'M', ACTION_MOVE},
    {'p', ACTION_PASTE}, {'P', ACTION_PASTE},
    {'r', ACTION_DELETE}, {'R', ACTION_DELETE},
    {'n', ACTION_NEW_FILE},
    {'N', ACTION_NEW_DIR},
    {' ', ACTION_SELECT},
    {1, ACTION_SELECT_ALL},
    {21, ACTION_SELECT_CLEAR},
    {'/', ACTION_FILTER},
    {KEY_PPAGE, ACTION_PAGE_UP},
    {KEY_NPAGE, ACTION_PAGE_DOWN},
    {KEY_HOME, ACTION_GOTO_TOP},
    {KEY_END, ACTION_GOTO_BOTTOM},
    {'g', ACTION_GOTO_TOP},
    {'G', ACTION_GOTO_BOTTOM},
    {0, ACTION_NONE}
};

void init_colors() {
    if (!has_colors() || !color_enabled) return;
    start_color();
    use_default_colors();
    ColorScheme *s = &schemes[current_scheme];
    init_pair(1, s->border, -1);
    init_pair(2, s->title, -1);
    init_pair(3, s->dir, -1);
    init_pair(4, s->file, -1);
    init_pair(5, s->highlight, -1);
    init_pair(6, s->text, -1);
    init_pair(7, s->status, -1);
    init_pair(8, COLOR_BLACK, s->highlight);
    init_pair(9, s->danger, -1);
    init_pair(10, s->success, -1);
    init_pair(11, s->info, -1);
    init_pair(12, COLOR_MAGENTA, -1);
    init_pair(13, s->warning, -1);
}

void draw_box(int y, int x, int h, int w, int col) {
    if (color_enabled) attron(COLOR_PAIR(col));
    mvprintw(y, x, CORNER_TL);
    for (int i = 0; i < w-2; i++) addstr(H_LINE);
    addstr(CORNER_TR);
    for (int i = 1; i < h-1; i++) {
        mvprintw(y+i, x, V_LINE);
        mvprintw(y+i, x+w-1, V_LINE);
    }
    mvprintw(y+h-1, x, CORNER_BL);
    for (int i = 0; i < w-2; i++) addstr(H_LINE);
    addstr(CORNER_BR);
    if (color_enabled) attroff(COLOR_PAIR(col));
}

void format_size(off_t size, char *buf, size_t len) {
    const char *u[] = {"B", "K", "M", "G", "T"};
    int unit = 0;
    double d = (double)size;
    while (d >= 1024 && unit < 4) { d /= 1024; unit++; }
    if (unit == 0) snprintf(buf, len, "%ld%s", size, u[unit]);
    else snprintf(buf, len, "%.1f%s", d, u[unit]);
}

void status_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(status_msg, sizeof(status_msg), fmt, args);
    va_end(args);
    status_is_error = 1;
}

void status_info(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(status_msg, sizeof(status_msg), fmt, args);
    va_end(args);
    status_is_error = 0;
}

void status_clear() {
    status_msg[0] = '\0';
    status_is_error = 0;
}

int confirm_dialog(const char *msg, const char *item) {
    int ch, sel = 1;
    while (1) {
        clear();
        int my, mx;
        getmaxyx(stdscr, my, mx);
        int bw = 60, bh = 8;
        int sx = (mx-bw)/2, sy = (my-bh)/2;
        
        draw_box(sy, sx, bh, bw, 9);
        if (color_enabled) attron(COLOR_PAIR(9)|A_BOLD);
        mvprintw(sy+2, sx+3, "%s", msg);
        if (color_enabled) attroff(COLOR_PAIR(9)|A_BOLD);
        if (color_enabled) attron(COLOR_PAIR(11)|A_BOLD);
        mvprintw(sy+3, sx+5, "%s", item);
        if (color_enabled) attroff(COLOR_PAIR(11)|A_BOLD);
        
        int by = sy+5;
        if (sel == 0) {
            if (color_enabled) attron(COLOR_PAIR(8));
            mvprintw(by, sx+15, " [ HAYIR ] ");
            if (color_enabled) attroff(COLOR_PAIR(8));
            if (color_enabled) attron(COLOR_PAIR(6));
            mvprintw(by, sx+35, "   EVET   ");
            if (color_enabled) attroff(COLOR_PAIR(6));
        } else {
            if (color_enabled) attron(COLOR_PAIR(6));
            mvprintw(by, sx+15, "   HAYIR   ");
            if (color_enabled) attroff(COLOR_PAIR(6));
            if (color_enabled) attron(COLOR_PAIR(8));
            mvprintw(by, sx+35, " [ EVET ] ");
            if (color_enabled) attroff(COLOR_PAIR(8));
        }
        refresh();
        ch = getch();
        if (ch == KEY_LEFT || ch == KEY_RIGHT) sel = !sel;
        else if (ch == 10) return sel;
        else if (ch == 27 || ch == 'n' || ch == 'N') return 0;
        else if (ch == 'y' || ch == 'Y') return 1;
    }
}

int input_dialog(const char *title, char *buf, int max, int is_dir) {
    int ch, pos = 0;
    buf[0] = '\0';
    while (1) {
        clear();
        int my, mx;
        getmaxyx(stdscr, my, mx);
        int bw = 60, bh = 7;
        int sx = (mx-bw)/2, sy = (my-bh)/2;
        
        draw_box(sy, sx, bh, bw, is_dir ? 3 : 4);
        if (color_enabled) attron(COLOR_PAIR(2)|A_BOLD);
        mvprintw(sy+1, sx+3, "%s", title);
        if (color_enabled) attroff(COLOR_PAIR(2)|A_BOLD);
        
        if (color_enabled) attron(COLOR_PAIR(6));
        mvprintw(sy+3, sx+3, "[");
        for (int i = 0; i < bw-8; i++) addstr("-");
        addstr("]");
        mvprintw(sy+3, sx+4, "%s", buf);
        if (color_enabled) attroff(COLOR_PAIR(6));
        
        if (color_enabled) attron(COLOR_PAIR(11));
        mvprintw(sy+5, sx+3, "Enter:Onay | ESC:Iptal");
        if (color_enabled) attroff(COLOR_PAIR(11));
        
        move(sy+3, sx+4+pos);
        refresh();
        ch = getch();
        
        if (ch == 27) return 0;
        else if (ch == 10 && pos > 0) return 1;
        else if ((ch == KEY_BACKSPACE || ch == 127 || ch == '\b') && pos > 0) buf[--pos] = '\0';
        else if (pos < max-1 && ch >= 32 && ch < 127) {
            buf[pos++] = ch;
            buf[pos] = '\0';
        }
    }
}

void info_dialog(const char *msg, int success) {
    clear();
    int my, mx;
    getmaxyx(stdscr, my, mx);
    int bw = 50, bh = 5;
    int sx = (mx-bw)/2, sy = (my-bh)/2;
    draw_box(sy, sx, bh, bw, success ? 10 : 9);
    if (color_enabled) attron(COLOR_PAIR(success ? 10 : 9)|A_BOLD);
    mvprintw(sy+2, sx+(bw-strlen(msg))/2, "%s", msg);
    if (color_enabled) attroff(COLOR_PAIR(success ? 10 : 9)|A_BOLD);
    refresh();
    getch();
}

void apply_filter() {
    app.select_count = 0;
    for (int i = 0; i < app.n_options; i++) {
        if (app.filter_active && app.filter[0]) {
            app.entries[i].visible = (strcasestr(app.entries[i].name, app.filter) != NULL);
        } else {
            app.entries[i].visible = 1;
        }
        if (app.entries[i].visible && app.entries[i].selected) app.select_count++;
    }
    int visible_count = 0;
    for (int i = 0; i < app.n_options; i++) if (app.entries[i].visible) visible_count++;
    app.page_count = (visible_count + PAGE_SIZE - 1) / PAGE_SIZE;
    if (app.page_count == 0) app.page_count = 1;
    app.page_start = 0;
    app.highlight = 0;
}

void* safe_malloc(size_t size, const char *ctx) {
    void *p = malloc(size);
    if (!p) {
        status_error("Bellek yetersiz: %s", ctx);
        return NULL;
    }
    return p;
}

void* safe_realloc(void *p, size_t size, const char *ctx) {
    void *np = realloc(p, size);
    if (!np) {
        status_error("Bellek yetersiz: %s", ctx);
        return NULL;
    }
    return np;
}

int remove_recursive(const char *path) {
    struct stat st;
    if (stat(path, &st) < 0) return -1;
    if (S_ISDIR(st.st_mode)) {
        DIR *d = opendir(path);
        if (!d) return -1;
        struct dirent *dir;
        while ((dir = readdir(d)) != NULL) {
            if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) continue;
            char child_path[MAX_PATH];
            snprintf(child_path, sizeof(child_path), "%s/%s", path, dir->d_name);
            if (remove_recursive(child_path) < 0) { closedir(d); return -1; }
        }
        closedir(d);
        return rmdir(path);
    } else {
        return unlink(path);
    }
}

int load_directory() {
    if (app.entries) {
        free(app.entries);
        app.entries = NULL;
    }
    
    DIR *d = opendir(app.current_dir);
    if (!d) {
        status_error("Dizin acilamadi: %s", strerror(errno));
        return 0;
    }
    
    app.entries = safe_malloc(sizeof(FileEntry) * 1000, "init entries");
    if (!app.entries) {
        closedir(d);
        return 0;
    }
    int capacity = 1000;
    
    struct dirent *dir;
    app.n_options = 0;
    
    while ((dir = readdir(d)) != NULL) {
        if (strcmp(dir->d_name, ".") == 0) continue;
        
        if (app.n_options >= capacity) {
            capacity *= 2;
            FileEntry *new_entries = safe_realloc(app.entries, sizeof(FileEntry) * capacity, "expand entries");
            if (!new_entries) {
                closedir(d);
                return 0;
            }
            app.entries = new_entries;
        }
        
        strncpy(app.entries[app.n_options].name, dir->d_name, 255);
        app.entries[app.n_options].name[255] = '\0';
        
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", app.current_dir, dir->d_name);
        
        struct stat st;
        if (stat(full_path, &st) == 0) {
            app.entries[app.n_options].is_dir = S_ISDIR(st.st_mode);
            app.entries[app.n_options].size = st.st_size;
        } else {
            app.entries[app.n_options].is_dir = 0;
            app.entries[app.n_options].size = 0;
        }
        
        app.entries[app.n_options].selected = 0;
        app.entries[app.n_options].visible = 1;
        app.n_options++;
    }
    
    closedir(d);
    apply_filter();
    status_clear();
    return 1;
}

void draw_ui() {
    clear();
    int my, mx;
    getmaxyx(stdscr, my, mx);
    
    draw_box(0, 0, 3, mx, 1);
    if (color_enabled) attron(COLOR_PAIR(2)|A_BOLD);
    mvprintw(1, 2, "[ %s ]", app.current_dir);
    if (color_enabled) attroff(COLOR_PAIR(2)|A_BOLD);
    
    char count_str[32];
    int visible_count = 0;
    for (int i = 0; i < app.n_options; i++) if (app.entries[i].visible) visible_count++;
    snprintf(count_str, sizeof(count_str), "%d files", visible_count);
    if (color_enabled) attron(COLOR_PAIR(11));
    mvprintw(1, mx - strlen(count_str) - 3, "%s", count_str);
    if (color_enabled) attroff(COLOR_PAIR(11));
    
    int info_x = 2;
    if (clipboard_count > 0) {
        if (color_enabled) attron(COLOR_PAIR(12)|A_BOLD);
        mvprintw(2, info_x, "[CLIP:%d %s]", clipboard_count, clipboard[0].is_cut ? "MV" : "CP");
        if (color_enabled) attroff(COLOR_PAIR(12)|A_BOLD);
        info_x += 15;
    }
    if (app.select_count > 0) {
        if (color_enabled) attron(COLOR_PAIR(13)|A_BOLD);
        mvprintw(2, info_x, "[SEL:%d]", app.select_count);
        if (color_enabled) attroff(COLOR_PAIR(13)|A_BOLD);
        info_x += 10;
    }
    if (app.filter_active) {
        if (color_enabled) attron(COLOR_PAIR(11)|A_BOLD);
        mvprintw(2, mx - strlen(app.filter) - 10, "[/%s]", app.filter);
        if (color_enabled) attroff(COLOR_PAIR(11)|A_BOLD);
    }
    
    if (app.page_count > 1) {
        char page_str[32];
        snprintf(page_str, sizeof(page_str), "Page %d/%d", (app.page_start/PAGE_SIZE)+1, app.page_count);
        mvprintw(2, (mx - strlen(page_str))/2, "%s", page_str);
    }
    
    int content_h = my - 6;
    draw_box(3, 0, content_h, mx, 1);
    
    int start_row = 4;
    int name_width = mx - 20;
    
    for (int i = 0; i < app.n_options && start_row < my - 3; i++) {
        if (!app.entries[i].visible) continue;
        
        int vis_idx = 0;
        for (int j = 0; j < i; j++) if (app.entries[j].visible) vis_idx++;
        if (vis_idx < app.page_start) continue;
        if (vis_idx >= app.page_start + PAGE_SIZE) break;
        
        char size_str[10];
        format_size(app.entries[i].size, size_str, sizeof(size_str));
        
        char sel_mark[4] = "  ";
        if (app.entries[i].selected) strcpy(sel_mark, "* ");
        
        if (i == app.highlight) {
            if (color_enabled) attron(COLOR_PAIR(8));
            mvprintw(start_row, 1, "%s>", sel_mark);
            mvprintw(start_row, 4, "%-*s", name_width - 3, "");
            if (color_enabled) attroff(COLOR_PAIR(8));
            
            if (color_enabled) attron(COLOR_PAIR(8));
            mvprintw(start_row, 4, "%s %s", app.entries[i].is_dir ? "[DIR]" : "[FIL]", app.entries[i].name);
            mvprintw(start_row, mx - 10, "%8s", size_str);
            if (color_enabled) attroff(COLOR_PAIR(8));
        } else {
            mvprintw(start_row, 1, "%s ", sel_mark);
            if (color_enabled) {
                if (app.entries[i].selected) attron(COLOR_PAIR(13));
                else attron(COLOR_PAIR(app.entries[i].is_dir ? 3 : 4));
            }
            mvprintw(start_row, 4, "%s %s", app.entries[i].is_dir ? "[DIR]" : "[FIL]", app.entries[i].name);
            if (color_enabled) {
                if (app.entries[i].selected) attroff(COLOR_PAIR(13));
                else attroff(COLOR_PAIR(app.entries[i].is_dir ? 3 : 4));
            }
            if (color_enabled) attron(COLOR_PAIR(11));
            mvprintw(start_row, mx - 10, "%8s", size_str);
            if (color_enabled) attroff(COLOR_PAIR(11));
        }
        start_row++;
    }
    
    draw_box(my-3, 0, 3, mx, status_is_error ? 9 : 7);
    if (status_msg[0]) {
        if (color_enabled) attron(COLOR_PAIR(status_is_error ? 9 : 7)|A_BOLD);
        mvprintw(my-2, 2, "%s", status_msg);
        if (color_enabled) attroff(COLOR_PAIR(status_is_error ? 9 : 7)|A_BOLD);
    } else {
        if (color_enabled) attron(COLOR_PAIR(7));
        mvprintw(my-2, 2, "c:Copy m:Move p:Paste r:Del n:NewF N:NewD Space:Sel A:All U:Clr /:Filt Pg:Page q:Quit");
        if (color_enabled) attroff(COLOR_PAIR(7));
    }
    
    refresh();
}

void filter_mode() {
    app.filter_active = 1;
    app.filter[0] = '\0';
    int pos = 0;
    int ch;
    
    while (1) {
        snprintf(status_msg, sizeof(status_msg), "Filter: %s_", app.filter);
        status_is_error = 0;
        draw_ui();
        
        ch = getch();
        
        if (ch == 27) {
            app.filter_active = 0;
            app.filter[0] = '\0';
            status_clear();
            apply_filter();
            return;
        } else if (ch == 10) {
            status_clear();
            return;
        } else if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
            if (pos > 0) app.filter[--pos] = '\0';
        } else if (pos < MAX_FILTER_LEN-1 && ch >= 32 && ch < 127) {
            app.filter[pos++] = ch;
            app.filter[pos] = '\0';
        }
        apply_filter();
    }
}

int copy_file(const char *src, const char *dst) {
    int fd_src = open(src, O_RDONLY);
    if (fd_src < 0) return -1;
    struct stat st;
    fstat(fd_src, &st);
    int fd_dst = open(dst, O_WRONLY|O_CREAT|O_TRUNC, st.st_mode);
    if (fd_dst < 0) { close(fd_src); return -1; }
    
    off_t offset = 0;
    ssize_t sent = sendfile(fd_dst, fd_src, &offset, st.st_size);
    close(fd_src);
    close(fd_dst);
    return (sent == st.st_size) ? 0 : -1;
}

int copy_dir_recursive(const char *src, const char *dst) {
    struct stat st;
    if (stat(src, &st) < 0) return -1;
    if (mkdir(dst, st.st_mode) < 0 && errno != EEXIST) return -1;
    
    DIR *d = opendir(src);
    if (!d) return -1;
    
    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) {
        if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) continue;
        
        char src_p[MAX_PATH], dst_p[MAX_PATH];
        snprintf(src_p, sizeof(src_p), "%s/%s", src, dir->d_name);
        snprintf(dst_p, sizeof(dst_p), "%s/%s", dst, dir->d_name);
        
        struct stat s;
        if (stat(src_p, &s) == 0) {
            if (S_ISDIR(s.st_mode)) {
                if (copy_dir_recursive(src_p, dst_p) < 0) { closedir(d); return -1; }
            } else {
                if (copy_file(src_p, dst_p) < 0) { closedir(d); return -1; }
            }
        }
    }
    closedir(d);
    return 0;
}

void execute_batch(int is_cut) {
    if (clipboard_count == 0) {
        status_error("Clipboard bos!");
        return;
    }
    
    int success = 0, fail = 0;
    
    for (int i = 0; i < clipboard_count; i++) {
        snprintf(status_msg, sizeof(status_msg), "[%d/%d] %s...", i+1, clipboard_count, clipboard[i].name);
        status_is_error = 0;
        draw_ui();
        refresh();
        
        char dst[MAX_PATH];
        snprintf(dst, sizeof(dst), "%s/%s", app.current_dir, clipboard[i].name);
        
        if (strcmp(clipboard[i].path, dst) == 0) continue;
        
        struct stat st;
        if (stat(dst, &st) == 0) {
            char msg[512];
            snprintf(msg, sizeof(msg), "'%s' uzerine yazilsin mi?", clipboard[i].name);
            if (!confirm_dialog("Dosya var", msg)) continue;
        }
        
        int res;
        if (clipboard[i].is_dir) {
            if (is_cut) res = rename(clipboard[i].path, dst);
            else res = copy_dir_recursive(clipboard[i].path, dst);
        } else {
            if (is_cut) res = rename(clipboard[i].path, dst);
            else res = copy_file(clipboard[i].path, dst);
        }
        
        if (res == 0) success++;
        else fail++;
    }
    
    if (is_cut) clipboard_count = 0;
    
    if (fail == 0) status_info("%d oge islemdi", success);
    else status_info("%d basari, %d basarisiz", success, fail);
}

Action get_action(int ch) {
    for (int i = 0; keymap[i].key != 0; i++) {
        if (keymap[i].key == ch) return keymap[i].action;
    }
    return ACTION_NONE;
}

void bios_style_menu() {
    MenuTab current_tab = TAB_COLORS;
    int color_highlight = 0;
    int ch;
    const char *color_names[] = {"Default", "Ocean", "Forest", "Sunset", "Matrix", "Mono", "Gold", "Purple"};
    
    while (1) {
        clear();
        int my, mx;
        getmaxyx(stdscr, my, mx);
        int box_w = 60, box_h = 20;
        int sx = (mx - box_w) / 2, sy = (my - box_h) / 2;
        
        draw_box(sy, sx, box_h, box_w, 1);
        if (color_enabled) attron(COLOR_PAIR(2) | A_BOLD);
        mvprintw(sy + 1, sx + 20, "SETTINGS");
        if (color_enabled) attroff(COLOR_PAIR(2) | A_BOLD);
        
        int tab_y = sy + 3, tab_x = sx + 4;
        for (int i = 0; i < NUM_TABS; i++) {
            if (i == current_tab) {
                draw_box(tab_y - 1, tab_x + (i * 12), 3, 12, 2);
                if (color_enabled) attron(COLOR_PAIR(2) | A_BOLD | A_REVERSE);
                mvprintw(tab_y, tab_x + 2 + (i * 12), " %s ", tab_names[i]);
                if (color_enabled) attroff(COLOR_PAIR(2) | A_BOLD | A_REVERSE);
            } else {
                if (color_enabled) attron(COLOR_PAIR(6));
                mvprintw(tab_y, tab_x + 2 + (i * 12), " %s ", tab_names[i]);
                if (color_enabled) attroff(COLOR_PAIR(6));
            }
        }
        
        mvprintw(tab_y + 2, sx + 2, "------------------------------------------");
        int content_y = tab_y + 4;
        
        switch (current_tab) {
            case TAB_COLORS:
                if (color_enabled) attron(COLOR_PAIR(2) | A_BOLD);
                mvprintw(content_y, sx + 4, "Color Scheme:");
                if (color_enabled) attroff(COLOR_PAIR(2) | A_BOLD);
                
                for (int i = 0; i < 8; i++) {
                    int row = content_y + 2 + i;
                    if (i == color_highlight) {
                        if (color_enabled) attron(COLOR_PAIR(8));
                        mvprintw(row, sx + 4, ">>");
                        if (color_enabled) attroff(COLOR_PAIR(8));
                    } else mvprintw(row, sx + 4, "  ");
                    
                    if (i == current_scheme) {
                        if (color_enabled) attron(COLOR_PAIR(10));
                        mvprintw(row, sx + 7, "* ");
                        if (color_enabled) attroff(COLOR_PAIR(10));
                    } else mvprintw(row, sx + 7, "  ");
                    
                    if (color_enabled) attron(COLOR_PAIR(schemes[i].title));
                    if (i == color_highlight) attron(A_BOLD);
                    mvprintw(row, sx + 10, "%s", color_names[i]);
                    if (i == color_highlight) attroff(A_BOLD);
                    if (color_enabled) attroff(COLOR_PAIR(schemes[i].title));
                    
                    if (color_enabled) attron(COLOR_PAIR(schemes[i].border));
                    mvprintw(row, sx + 25, "####");
                    if (color_enabled) attroff(COLOR_PAIR(schemes[i].border));
                }
                
                mvprintw(content_y + 12, sx + 4, "--------------------------------------");
                if (color_enabled) attron(COLOR_PAIR(11));
                mvprintw(content_y + 13, sx + 4, "Up/Down:Select | Enter:Apply | Left/Right:Tab");
                if (color_enabled) attroff(COLOR_PAIR(11));
                break;
                
            case TAB_VIEW:
            case TAB_KEYS:
                if (color_enabled) attron(COLOR_PAIR(11));
                mvprintw(content_y + 5, sx + 15, "... Coming soon ...");
                if (color_enabled) attroff(COLOR_PAIR(11));
                break;
                
            case TAB_ABOUT:
                if (color_enabled) attron(COLOR_PAIR(2) | A_BOLD);
                mvprintw(content_y + 2, sx + 22, "DRMNGR v4");
                if (color_enabled) attroff(COLOR_PAIR(2) | A_BOLD);
                if (color_enabled) attron(COLOR_PAIR(6));
                mvprintw(content_y + 4, sx + 10, "Terminal File Manager");
                mvprintw(content_y + 6, sx + 8, "Built with ncurses | MIT License");
                mvprintw(content_y + 8, sx + 12, "github.com/JamesAnanenkovic");
                if (color_enabled) attroff(COLOR_PAIR(6));
                break;
        }
        
        mvprintw(sy + box_h - 3, sx + 2, "------------------------------------------");
        if (color_enabled) attron(COLOR_PAIR(7));
        mvprintw(sy + box_h - 2, sx + 4, "ESC:Close | TAB:Next Tab");
        if (color_enabled) attroff(COLOR_PAIR(7));
        
        refresh();
        ch = getch();
        
        switch (ch) {
            case 27: init_colors(); return;
            case KEY_LEFT: if (current_tab > 0) current_tab--; break;
            case KEY_RIGHT: case '\t': current_tab = (current_tab + 1) % NUM_TABS; break;
            case KEY_UP: if (current_tab == TAB_COLORS && color_highlight > 0) color_highlight--; break;
            case KEY_DOWN: if (current_tab == TAB_COLORS && color_highlight < 7) color_highlight++; break;
            case 10: case ' ':
                if (current_tab == TAB_COLORS) {
                    current_scheme = color_highlight;
                    color_enabled = 1;
                    init_colors();
                }
                break;
        }
    }
}

void handle_action(Action act) {
    switch (act) {
        case ACTION_UP: {
            for (int i = app.highlight - 1; i >= 0; i--) {
                if (app.entries[i].visible) {
                    app.highlight = i;
                    int vis_idx = 0;
                    for (int j = 0; j < i; j++) if (app.entries[j].visible) vis_idx++;
                    if (vis_idx < app.page_start) {
                        app.page_start -= PAGE_SIZE;
                        if (app.page_start < 0) app.page_start = 0;
                    }
                    break;
                }
            }
            break;
        }
        case ACTION_DOWN: {
            for (int i = app.highlight + 1; i < app.n_options; i++) {
                if (app.entries[i].visible) {
                    app.highlight = i;
                    int vis_idx = 0;
                    for (int j = 0; j < i; j++) if (app.entries[j].visible) vis_idx++;
                    if (vis_idx >= app.page_start + PAGE_SIZE) {
                        app.page_start += PAGE_SIZE;
                    }
                    break;
                }
            }
            break;
        }
        case ACTION_LEFT:
            if (chdir("..") == 0) {
                getcwd(app.current_dir, sizeof(app.current_dir));
                load_directory();
            } else {
                status_error("Ust dizine gidilemedi");
            }
            break;
        case ACTION_RIGHT:
        case ACTION_ENTER:
            if (app.n_options > 0 && app.entries[app.highlight].is_dir) {
                if (chdir(app.entries[app.highlight].name) == 0) {
                    getcwd(app.current_dir, sizeof(app.current_dir));
                    load_directory();
                } else {
                    status_error("Dizin acilamadi");
                }
            }
            break;
        case ACTION_SELECT: {
            if (app.n_options == 0) break;
            app.entries[app.highlight].selected = !app.entries[app.highlight].selected;
            if (app.entries[app.highlight].selected) app.select_count++;
            else app.select_count--;
            handle_action(ACTION_DOWN);
            break;
        }
        case ACTION_SELECT_ALL: {
            int all_selected = (app.select_count > 0);
            app.select_count = 0;
            for (int i = 0; i < app.n_options; i++) {
                if (app.entries[i].visible) {
                    app.entries[i].selected = !all_selected;
                    if (app.entries[i].selected) app.select_count++;
                }
            }
            status_info(all_selected ? "Secim temizlendi" : "Tumu secildi");
            break;
        }
        case ACTION_SELECT_CLEAR: {
            for (int i = 0; i < app.n_options; i++) app.entries[i].selected = 0;
            app.select_count = 0;
            status_info("Secim temizlendi");
            break;
        }
        case ACTION_COPY: {
            int added = 0;
            for (int i = 0; i < app.n_options && clipboard_count < CLIPBOARD_SIZE; i++) {
                if (app.entries[i].selected || i == app.highlight) {
                    int dup = 0;
                    for (int j = 0; j < clipboard_count; j++) {
                        if (strcmp(clipboard[j].name, app.entries[i].name) == 0) { dup = 1; break; }
                    }
                    if (!dup) {
                        snprintf(clipboard[clipboard_count].path, MAX_PATH, "%s/%s", app.current_dir, app.entries[i].name);
                        strncpy(clipboard[clipboard_count].name, app.entries[i].name, 256);
                        clipboard[clipboard_count].is_dir = app.entries[i].is_dir;
                        clipboard[clipboard_count].is_cut = 0;
                        clipboard[clipboard_count].active = 1;
                        clipboard_count++;
                        added++;
                    }
                }
            }
            for (int i = 0; i < app.n_options; i++) app.entries[i].selected = 0;
            app.select_count = 0;
            status_info("%d oge kopyalandi", added);
            break;
        }
        case ACTION_MOVE: {
            int new_count = 0;
            for (int i = 0; i < clipboard_count; i++) {
                if (!clipboard[i].is_cut) clipboard[new_count++] = clipboard[i];
            }
            clipboard_count = new_count;
            
            int added = 0;
            for (int i = 0; i < app.n_options && clipboard_count < CLIPBOARD_SIZE; i++) {
                if (app.entries[i].selected || i == app.highlight) {
                    snprintf(clipboard[clipboard_count].path, MAX_PATH, "%s/%s", app.current_dir, app.entries[i].name);
                    strncpy(clipboard[clipboard_count].name, app.entries[i].name, 256);
                    clipboard[clipboard_count].is_dir = app.entries[i].is_dir;
                    clipboard[clipboard_count].is_cut = 1;
                    clipboard[clipboard_count].active = 1;
                    clipboard_count++;
                    added++;
                }
            }
            for (int i = 0; i < app.n_options; i++) app.entries[i].selected = 0;
            app.select_count = 0;
            status_info("%d oge tasinmaya hazir", added);
            break;
        }
        case ACTION_PASTE:
            if (clipboard_count == 0) {
                status_error("Clipboard bos!");
            } else {
                execute_batch(clipboard[0].is_cut);
                load_directory();
            }
            break;
        case ACTION_DELETE: {
            int del_count = 0;
            for (int i = 0; i < app.n_options; i++) {
                if (app.entries[i].selected || i == app.highlight) {
                    char msg[512], item[300];
                    snprintf(msg, sizeof(msg), "Silmek istediginize emin misiniz?");
                    snprintf(item, sizeof(item), "%s %s", app.entries[i].is_dir ? "[DIR]" : "[FIL]", app.entries[i].name);
                    
                    if (confirm_dialog(msg, item)) {
                        char path[MAX_PATH];
                        snprintf(path, sizeof(path), "%s/%s", app.current_dir, app.entries[i].name);
                        if (remove_recursive(path) == 0) del_count++;
                    }
                }
            }
            if (del_count > 0) {
                status_info("%d oge silindi", del_count);
                load_directory();
            }
            break;
        }
        case ACTION_NEW_FILE: {
            char buf[256];
            if (input_dialog("Yeni Dosya:", buf, sizeof(buf), 0)) {
                char path[MAX_PATH];
                snprintf(path, sizeof(path), "%s/%s", app.current_dir, buf);
                FILE *f = fopen(path, "w");
                if (f) { fclose(f); status_info("Dosya olusturuldu"); load_directory(); }
                else status_error("Dosya olusturulamadi");
            }
            break;
        }
        case ACTION_NEW_DIR: {
            char buf[256];
            if (input_dialog("Yeni Klasor:", buf, sizeof(buf), 1)) {
                char path[MAX_PATH];
                snprintf(path, sizeof(path), "%s/%s", app.current_dir, buf);
                if (mkdir(path, 0755) == 0) { status_info("Klasor olusturuldu"); load_directory(); }
                else status_error("Klasor olusturulamadi");
            }
            break;
        }
        case ACTION_FILTER:
            filter_mode();
            break;
        case ACTION_PAGE_UP:
            if (app.page_start >= PAGE_SIZE) {
                app.page_start -= PAGE_SIZE;
                int vis = 0;
                for (int i = 0; i < app.n_options; i++) {
                    if (!app.entries[i].visible) continue;
                    if (vis == app.page_start) { app.highlight = i; break; }
                    vis++;
                }
            }
            break;
        case ACTION_PAGE_DOWN:
            if (app.page_start + PAGE_SIZE < app.n_options) {
                app.page_start += PAGE_SIZE;
                int vis = 0;
                for (int i = 0; i < app.n_options; i++) {
                    if (!app.entries[i].visible) continue;
                    if (vis == app.page_start) { app.highlight = i; break; }
                    vis++;
                }
            }
            break;
        case ACTION_GOTO_TOP:
            app.page_start = 0;
            for (int i = 0; i < app.n_options; i++) {
                if (app.entries[i].visible) { app.highlight = i; break; }
            }
            break;
        case ACTION_GOTO_BOTTOM:
            app.page_start = (app.page_count - 1) * PAGE_SIZE;
            for (int i = app.n_options - 1; i >= 0; i--) {
                if (app.entries[i].visible) { app.highlight = i; break; }
            }
            break;
        case ACTION_QUIT:
            endwin();
            if (app.entries) free(app.entries);
            exit(0);
            break;
        default:
            break;
    }
}

int main() {
    getcwd(app.current_dir, sizeof(app.current_dir));
    
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    init_colors();
    
    if (!load_directory()) {
        endwin();
        fprintf(stderr, "Baslangic dizini yuklenemedi\n");
        return 1;
    }
    
    while (1) {
        draw_ui();
        int ch = getch();
        
        if (ch == 27) {
            nodelay(stdscr, TRUE);
            int next = getch();
            nodelay(stdscr, FALSE);
            
            if (next == -1) {
                bios_style_menu();
                clear();
            } else {
                ungetch(next);
            }
        } else {
            Action act = get_action(ch);
            if (act != ACTION_NONE) {
                handle_action(act);
            }
        }
    }
    
    return 0;
}
