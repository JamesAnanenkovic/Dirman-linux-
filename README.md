# Dirman-linux v3 🚀

**Lightweight Terminal File Manager for Linux**

A fast, keyboard-driven file manager with a beautiful TUI (Terminal User Interface) built with ncurses. No mouse needed - pure efficiency!


## ✨ Features

### Core Navigation
- **Keyboard-only control** - Navigate faster than GUI file managers
- **Vim-style bindings** - `j/k` or arrow keys for movement
- **Quick directory jumping** - `Enter` to enter, `-` to go up
- **Visual file tree** - ASCII box drawing with color themes

### File Operations
| Key | Action | Details |
|-----|--------|---------|
| `c` | **Copy** | Add file/folder to clipboard |
| `m` | **Move** | Cut file/folder for relocation |
| `p` | **Paste** | Execute copy/move operations |
| `r` | **Delete** | Remove with confirmation dialog |
| `n` | **New File** | Create empty file |
| `N` | **New Folder** | Create new directory |

### Advanced Features
- **Multi-clipboard** - Copy up to 10 items at once
- **Batch operations** - Paste multiple files simultaneously
- **Human-readable sizes** - See `4.2K`, `1.5G`, `156B` instantly
- **Visual clipboard indicator** - `*` marks selected items
- **Recursive copy** - Folders copied with full contents
- **Safe operations** - Confirmations for delete/overwrite

### Customization
- **8 Color Schemes**: Default, Ocean, Forest, Sunset, Matrix, Mono, Gold, Purple
- **Toggle colors** - Disable for monochrome terminals
- **Real-time preview** - See changes instantly in settings menu

## 🎮 Controls
```
Navigation:
↑/↓ or j/k     Move selection
Enter          Open directory

             Go to parent directory

q              Quit
File Operations:
c              Copy to clipboard
m              Cut (move) to clipboard
p              Paste clipboard contents
r              Delete (with confirmation)
n              Create new file
N              Create new directory
System:
ESC            Open settings menu
Arrow Keys     Navigate menus
Enter/Space    Select option
```


## 📸 Screenshots
```
+----------------------------------------------------------+
[ /home/user/projects                                      ] [CLIPBOARD: 2 items (COPY)]
+----------------------------------------------------------+
|  [DIR] ..                                          4.0K  |
|  [DIR] src                                        12.3M  |
| *[DIR] assets                                      1.5G  |
| *[FIL] README.md                                   4.2K  |
|  [FIL] main.c                                      156B  |
|  [DIR] tests                                       8.9M  |
|  [FIL] Makefile                                     2.1K  |
+----------------------------------------------------------+
| ENTER:Open | c:Copy | m:Move | p:Paste | R:Del | ESC:Menu |
+----------------------------------------------------------+
```


## 🚀 Installation

## Debian/Ubuntu
```sudo apt install libncurses5-dev```

## Fedora/RHEL
```sudo dnf install ncurses-devel```

## Arch Linux
```sudo pacman -S ncurses```

# Build & Install

## Clone repository
```git clone https://github.com/JamesAnanenkovic/Dirman-linux-.git```
```cd Dirman-linux-```

## Compile
```gcc -o dirmanlinux dirmanlinux.c -lncurses```

## Install system-wide (optional)
```sudo cp dirmanlinux /usr/bin/```

# Run
```dirmanlinux```

🎨 Color Schemes
```
Press ESC → select theme:
Table
Theme	Style	Best For
Default	White/Cyan	General use
Ocean	Blue tones	Cool atmosphere
Forest	Green tones	Nature lovers
Sunset	Red/Yellow	Warm vibe
Matrix	Green on black	Hackerman mode
Mono	Grayscale	Minimalists
Gold	Yellow tones	Luxury feel
Purple	Magenta tones	Creative minds
```
⚡ Performance

   - Zero dependencies except ncurses
   - Single binary - < 50KB compiled
   - Instant startup - No loading time
   - Efficient copying - Uses sendfile() for speed
   - Low memory - Handles thousands of files

🛡️ Safety Features

   - Confirmation dialogs for destructive operations
   - Visual distinction between files and folders
   - Clipboard persistence until paste or clear
   - Cannot paste into same location (prevents duplicates)
   - Overwrite confirmation with file details

📝 Changelog
v3.0 (Current)

    ✅ Multi-item clipboard (copy/cut/paste)
    ✅ Human-readable file sizes
    ✅ Recursive directory copying
    ✅ 8 color schemes
    ✅ Batch operations
    ✅ Visual clipboard indicators

v2.0

    ✅ Settings menu (ESC)
    ✅ Color customization
    ✅ New file/folder creation
    ✅ Delete with confirmation

v1.0

    ✅ Basic navigation
    ✅ Directory listing
    ✅ ASCII UI

🤝 Contributing
Pull requests welcome! Areas for improvement:

    [ ] Search functionality (/)
    [ ] File preview (text files)
    [ ] Permissions editing
    [ ] Symlink handling
    [ ] Bookmarks/favorites

📄 License
MIT License - See LICENSE for details.
🙏 Credits
Built with passion and ncurses. Inspired by ranger, mc, and vim.

## Pro Tip: Add alias fm='dirmanlinux' to your .bashrc for quick access!
