# Drmngr v4 🚀

**Advanced Terminal File Manager for Linux**

[![Version](https://img.shields.io/badge/version-4.0-blue)](https://github.com/JamesAnanenkovic/Dirman-linux-)
[![License](https://img.shields.io/badge/license-MIT-green)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Linux-orange)](https://github.com/JamesAnanenkovic/Dirman-linux-)

A fast, keyboard-driven file manager with beautiful TUI. Handles 100k+ files with pagination, multi-selection, and batch operations.

## ✨ Features

| Feature | Description |
|---------|-------------|
| **Multi-Select** | Space toggle, Ctrl+A all, Ctrl+U clear |
| **Batch Operations** | Copy/Move/Delete multiple files at once |
| **Live Filter** | `/` to search, instant results |
| **Pagination** | 100 items/page, smooth 100k+ handling |
| **BIOS-Style Menu** | Tabbed settings with color preview |
| **Progress Indicators** | Real-time clipboard/selection/file counts |
| **Safe & Fast** | Error handling, sendfile() for speed |

## 🎮 Controls

### Navigation
| Key | Action |
|-----|--------|
| `↑↓` or `jk` | Move cursor |
| `Enter` or `l` | Open directory |
| `-` or `h` | Parent directory |
| `g` / `G` | Go to top/bottom |
| `PgUp` / `PgDn` | Previous/next page |

### File Operations
| Key | Action |
|-----|--------|
| `Space` | Select/deselect item |
| `Ctrl+A` | Select all visible |
| `Ctrl+U` | Clear selection |
| `c` | Copy to clipboard |
| `m` | Cut (move) to clipboard |
| `p` | Paste clipboard |
| `r` | Delete (with confirmation) |
| `n` | New file |
| `N` | New folder |
| `/` | Filter mode |

### System
| Key | Action |
|-----|--------|
| `ESC` | Settings menu |
| `q` | Quit |

## 🚀 Quick Start

# Clone
git clone https://github.com/JamesAnanenkovic/Dirman-linux-.git
cd Dirman-linux-

# Build & Install
chmod +x load.sh
./load.sh install

# Or just build locally
./load.sh
./drmngr

📦 Dependencies
# Debian/Ubuntu
sudo apt install libncurses5-dev

# Fedora/RHEL
sudo dnf install ncurses-devel

# Arch Linux
sudo pacman -S ncurses

🎨 Color Themes
Press ESC → Colors tab, select with arrow keys:
| Theme   | Style          |
| ------- | -------------- |
| Default | White/Cyan     |
| Ocean   | Blue tones     |
| Forest  | Green tones    |
| Sunset  | Red/Yellow     |
| Matrix  | Green terminal |
| Mono    | Grayscale      |
| Gold    | Yellow         |
| Purple  | Magenta        |


📁 Build Options
./load.sh          # Local build only
./load.sh install  # Build + system install
./load.sh clean    # Remove build files
./load.sh uninstall # Remove from system

Manual build:
gcc -o drmngr dirmanlinux.c -lncurses -O2
sudo cp drmngr /usr/bin/

🖥️ Interface
```
+----------------------------------------------------------+
[ /home/user/projects                               ] [42 files]
[CLIP:3 CP] [SEL:5]                          [Page 1/3] [/filter]
+----------------------------------------------------------+
| >> [DIR] src                                      12.3M  |
| *  [DIR] assets                                   1.5G   |
| *  [FIL] README.md                                4.2K   |
|    [FIL] main.c                                   156B   |
|    [DIR] tests                                    8.9M   |
+----------------------------------------------------------+
| c:Copy m:Move p:Paste r:Del n:NewF N:NewD Space:Sel A:All|
+----------------------------------------------------------+
```
📝 Changelog
v4.0 (Current)

    ✅ Multi-selection system (Space/Ctrl+A/Ctrl+U)
    ✅ Batch copy/move/delete operations
    ✅ Live filter with /
    ✅ Pagination (100 items/page)
    ✅ BIOS-style tabbed settings menu
    ✅ 8 color themes with preview
    ✅ Error handling & memory safety
    ✅ Progress indicators

v3.0

    Basic file operations
    Single clipboard
    ASCII UI

🤝 Contributing
PRs welcome! Roadmap:

    [ ] File preview (text/images)
    [ ] Search in file contents
    [ ] Favorites/bookmarks
    [ ] Permission editor
    [ ] Split pane view

📄 License
MIT License - see LICENSE
🙏 Credits
Built with ncurses and passion. Inspired by ranger, mc, vim.
Pro tip: Add alias fm='drmngr' to your .bashrc!
