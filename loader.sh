echo "Dirman is loading..."
gcc -o dirmanlinux dirmanlinux.c -lncurses
echo "Compiled sucsesfully!"
sudo cp dirmanlinux /usr/bin/
echo "Dirman is ready!"
