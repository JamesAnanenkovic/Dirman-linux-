echo "Dirman is loading..."
gcc -o drmngr dirmanlinux.c -lncurses
echo "Compiled sucsesfully!"
sudo cp drmngr /usr/bin/
echo "Dirman is ready!"
