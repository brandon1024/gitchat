# git-chat
[![Build Status](https://travis-ci.com/brandon1024/gitchat.svg?token=zYgs3LGRABLGhdeJPft1&branch=master)](https://travis-ci.com/brandon1024/gitchat)

`git-chat` is a Git-based command line messaging application. Version control systems were never built to be used as an messaging application, but Git is built such that it can be used as a chat application. So we set out to build this silly project.

The git-chat project is a silly experiment. The purpose of the project is to prove that Git can be used as a means of secure communication. Once installed, it can be run from the command line using `git chat [args]`.

If you're using git-chat for messaging with your savvy friends, you're an awesome person. But please do keep in mind that git-chat should not be used for secure and tamper proof communication, because it suffers from potential tampering (lost messages, erased history, etc.). Please use at your own risk.

## Runtime Dependencies
git-chat has very few dependencies. It only needs `Git` and `GPG` to be installed somewhere on the PATH.

## Building git-chat
This project uses the `CMake` build tool. We recommend version 3.7.2 or higher. This project also uses `CTest` for executing unit and integration tests, and optionally `Valgrind` for running memcheck.

We actively try to support as many environments as possible. This project can be compiled and run on Linux-based distributions, and MacOS, and can be compiled using GCC or Clang compilers. If we don't support your current system, let us know!

A note about Windows: We don't offer support for building or running git-chat on Windows machines, and we don't plan to support Windows any time soon.

### Installation
By default, git-chat is installed into your ~/bin and ~/share directories. To install, from the project root run:
```
$ mkdir build
$ cd build
$ cmake ..
$ make install

$ ~/bin/git-chat --version
```

For a global install, from the project root run:
```
$ mkdir build
$ cd build
$ cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..
$ make install

$ git-chat --version
```

To uninstall, use the make `uninstall` target:
```
$ make install

# undo installation
$ make uninstall
```

### Running Tests
The steps below are the very basics--just enough to get you started. For more details, see `test/README.md`.

Running unit tests does not require installation of git-chat. To run the unit tests, simply run from the project root:
```
$ mkdir build
$ cd build
$ cmake ..
$ make git-chat-unit-tests
$ ctest -R unit-tests --verbose
```

To run integration tests against a local installation of git-chat, run from the project root:
```
$ mkdir build
$ cd build
$ cmake ..
$ make all install
$ TEST_GIT_CHAT_INSTALLED=~/bin ctest -R integration-tests --verbose
```

To run integration tests against a global installation of git-chat, run from the project root:
```
$ mkdir build
$ cd build
$ cmake ..
$ make all install
$ ctest -R integration-tests --verbose
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