# git-chat
`git-chat` is a Git-based command line messaging application. Version control systems were never built to be used as an messaging application, but Git is built such that it can be used as a chat application. So we set out to build this silly project.

The git-chat project is a silly experiment. The purpose of the project is to prove that Git can be used as a means of secure communication. Once installed, it can be run from the command line using `git chat [args]`.

If you're using git-chat for messaging with your savvy friends, you're an awesome person. But please do keep in mind that git-chat should not be used for secure and tamper proof communication, because it suffers from potential tampering (lost messages, erased history, etc.). Please use at your own risk.

## Runtime Dependencies
git-chat has very few dependencies. It only needs `Git` and `GPG` to be installed somewhere on the PATH.

## Building git-chat
This project uses the `CMake` build tool. We recommend version 2.8.11. This project also uses `CTest` for executing unit and integration tests, and optionally `Valgrind` for running memcheck.

We actively try to support as many environments as possible. This project can be compiled and run on Linux-based distributions, and MacOS, and can be compiled using GCC or Clang compilers. If we don't support your current system, let us know!

A note about Windows: We don't offer support for building or running git-chat on Windows machines, and we don't plan to support Windows any time soon.

### Compiling
To compile git-chat from sources:
```
$ mkdir build
$ cd build
$ cmake ..
$ cmake --build .
```

### Installing
Installing git-chat will install the `git-chat` executable in `/usr/local/bin`, install template files in `/usr/local/share/git-chat`, and install man pages in `/usr/share/man/man1`.

To install git-chat:
```
$ mkdir build
$ cd build
$ cmake ..
$ sudo cmake --build . --target install
```

### Running Tests
To run unit tests:
```
$ mkdir build
$ cd build
$ cmake ..
$ cmake --build .
$ ctest -R unit-tests
```

To run unit tests with Valgrind memcheck:
```
$ mkdir build
$ cd build
$ cmake ..
$ cmake --build .
$ cd test
$ valgrind --tool=memcheck --gen-suppressions=all --leak-check=full \
        --leak-resolution=high --track-origins=yes --vgdb=no --error-exitcode=1 \
        ./git-chat-unit-tests
```

To run integration tests:
```
$ mkdir build
$ cd build
$ cmake ..
$ cmake --build .
$ ctest -R integration-tests
```

To run all tests:
```
$ mkdir build
$ cd build
$ cmake ..
$ cmake --build .
$ ctest
```

## Using git-chat
Git has a neat way of allowing third parties to create extensions to Git that can be run as if they were built-in. If an executable on the PATH is prefixed with `git-`, git will execute it if the subcommand matches what follows after the prefix. Since git-chat is installed as an executable `git-chat`, you can invoke git-chat by simply running `git chat [args]`.

### git chat init
Initialize a new messaging space. This will call 'git init' to initialize the repository, and then setup the repository to be used for messaging.

```
usage: git chat init [(-n | --name) <name>] [(-d | --description) <desc>]
   or: git chat init [-q | --quiet]
   or: git chat init (-h | --help)

        -n, --name <name>      Specify a name for the master channel
        -d, --description <desc>
                        Specify a description for the space
        -q, --quiet            Only print error and warning messages
        -h, --help             Show usage and exit
```

### git chat channel
Create a new channel by branching off the current point in the conversation.

### git chat message
Create a new message in the current channel. The message is encrypted with GPG. The message is not yet published, and needs to be pushed to the remote repository using `git chat publish`.

### git chat publish
Push a any new messages to the remote repository.

### git chat read
Read the messages in the current channel.

### git chat get
Fetch any new messages from the remote repository.

## Extending git-chat
git-chat follows a similar extension model to Git, where executables located on the PATH that are prefixed with `git-chat-` will be invoked when `git chat <extension name>` is run at the command line. This allows you to build custom plugins to git-chat, extending it to work for you!

## Contributors

|[<img src="https://avatars3.githubusercontent.com/u/22732449?v=3&s=460" width="128">](https://github.com/brandon1024)|[<img src="https://avatars1.githubusercontent.com/u/8900382?s=460&v=4" width="128">](https://github.com/omnibrian)
|:---:|:---:|
|[Brandon Richardson](https://github.com/brandon1024)| [Brian LeBlanc](https://github.com/omnibrian)