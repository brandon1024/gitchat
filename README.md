# gitchat
`git-chat` is a Git-based command line messaging application. Version control systems were never built to be used as an IRC, but Git is built such that it can be used as a chat application, and that's what we wished to build.

The git-chat project is a silly experiment. The purpose of the project is to prove that Git can be used as a means of secure communication. The project is entirely self contained, meaning that you don't need to have Git installed locally. Once installed, it can be run from the command line using `git chat [args]` if Git is installed, or `./git-chat [args]` if not.

If you're using git-chat for chatting with your savvy friends, you're an awesome person. But please do keep in mind that git-chat should not be used for secure and tamper proof communication because it suffers from tampering, i.e. lost messages, erased history, etc. Please use at your own risk.

## Compile
This project was designed to be built using cmake. We recommend version 2.8.11.

### Installing Dependencies
This project depends on libgit2. To install it, just clone the libgit2 project from GitHub into the `extern/` directory in the project, like so:

```
cd extern/
git clone git@github.com:libgit2/libgit2.git
```

You must also have OpenSSL and libSSH2 installed for libgit2 to build correctly. If you don't have them installed, the easiest way is to install using homebrew:

```
brew install openssl
brew install libssh2
```

This will install both dependencies to the homebrew Cellar, so you will need to make sure that you link the installation of openssl and libssh2 to your path, otherwise cmake will not find it. You can do this using [brew link](https://github.com/Homebrew/brew/blob/master/docs/FAQ.md#can-i-install-my-own-stuff-to-usrlocal), or add it to your $PATH. If using cmake through JetBrains CLion, [refer to this Stack Overflow answer for adding cmake environment variables](https://stackoverflow.com/a/38874446).


### Build Project
Once complete, the project should build using the following:

```
mkdir build
cd build
cmake ..
cmake --build .
```
