# gitchat
A Git-Based Command-Line Messaging Application

## Compiling using CMake
Make sure to have cmake >= 2.6 installed on your machine.

Build and install:
```bash
mkdir cmake-build-dir
cd cmake-build-dir
# Build makefile
cmake ..
# Run build
make
# Run install
sudo make install
```

## Compiling from sources
```
gcc -Wall usage.c channel.c get.c message.c publish.c read.c main.c -o git-chat
```

Run:
```
./git-chat <command> [<options>]

or

cp git-chat /usr/local/bin/git-chat
git chat <command> [<options>]
```
