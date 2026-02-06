# Dirman-linux
## Version 2

Basic & light terminal file manager for Linux.

## Features

- **Keyboard navigation** - Arrow keys or Vim-style (j/k)
- **Visual interface** - ASCII box drawing, minimal design
- **Directory operations** - Enter to open, `-` to go up
- **File/Folder detection** - Auto-detects type with [DIR]/[FIL] labels
- **Lightweight** - Single C file, ncurses only dependency

## Controls

| Key | Action |
|-----|--------|
| ↑ / k | Move up |
| ↓ / j | Move down |
| Enter | Open directory |
| - | Go to parent directory |
| q / Esc | Quit |

## Build

```gcc -o dirmanlinux dirmanlinux.c -lncurses ```

## Run

```./dirmanlinux```

## Requirements

-    **ncurses** library
-    **Linux**/Unix environment

### Debian/Ubuntu
```sudo apt install libncurses5-dev```

### Fedora/RHEL
```sudo dnf install ncurses-devel```

### Arch
```sudo pacman -S ncurses```

## License
### MIT
