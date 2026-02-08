#!/bin/bash

# Dirman-linux v4 - Build & Install Script

set -e

APP_NAME="drmngr"
SRC_FILE="dirmanlinux.c"
INSTALL_DIR="/usr/bin"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

print_status() { echo -e "${GREEN}[✓]${NC} $1"; }
print_error() { echo -e "${RED}[✗]${NC} $1"; }
print_info() { echo -e "${YELLOW}[i]${NC} $1"; }

check_deps() {
    print_info "Checking dependencies..."
    if ! pkg-config --exists ncurses 2>/dev/null; then
        if ! ldconfig -p | grep -q libncurses; then
            print_error "ncurses not found!"
            echo "Install: sudo apt install libncurses5-dev (or equivalent)"
            exit 1
        fi
    fi
    print_status "ncurses found"
}

build() {
    print_info "Building $APP_NAME..."
    
    if [ ! -f "$SRC_FILE" ]; then
        print_error "Source file not found: $SRC_FILE"
        exit 1
    fi
    
    gcc -o "$APP_NAME" "$SRC_FILE" -lncurses -Wall -O2 || {
        print_error "Build failed!"
        exit 1
    }
    
    print_status "Build successful"
}

install_system() {
    print_info "Installing to $INSTALL_DIR..."
    
    if [ -f "$INSTALL_DIR/$APP_NAME" ]; then
        print_info "Removing old version..."
        sudo rm -f "$INSTALL_DIR/$APP_NAME"
    fi
    
    sudo cp "$APP_NAME" "$INSTALL_DIR/"
    sudo chmod 755 "$INSTALL_DIR/$APP_NAME"
    print_status "Installed to $INSTALL_DIR/$APP_NAME"
}

uninstall() {
    print_info "Uninstalling..."
    if [ -f "$INSTALL_DIR/$APP_NAME" ]; then
        sudo rm -f "$INSTALL_DIR/$APP_NAME"
        print_status "Removed from $INSTALL_DIR"
    else
        print_error "Not found in $INSTALL_DIR"
    fi
}

clean() {
    print_info "Cleaning..."
    rm -f "$APP_NAME"
    print_status "Cleaned"
}

test_run() {
    print_info "Testing..."
    if [ -f "./$APP_NAME" ]; then
        print_status "Local build OK"
    fi
    
    if command -v "$APP_NAME" &> /dev/null; then
        print_status "System install verified"
        echo ""
        echo "Usage: $APP_NAME"
    else
        print_error "System install not found"
    fi
}

# Main
case "${1:-build}" in
    install)
        check_deps
        build
        install_system
        test_run
        ;;
    clean)
        clean
        ;;
    uninstall)
        uninstall
        ;;
    *)
        check_deps
        build
        echo ""
        print_status "Build complete: ./$APP_NAME"
        echo "Install: ./load.sh install"
        ;;
esac
