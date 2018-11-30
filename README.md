# git-chat
`git-chat` is a Git-based command line messaging application. Version control systems were never built to be used as an messaging application, but Git is built such that it can be used as a chat application. So we set out to build this silly project.

The git-chat project is a silly experiment. The purpose of the project is to prove that Git can be used as a means of secure communication. The project is entirely self contained, meaning that you don't need to have Git installed locally. Once installed, it can be run from the command line using `git chat [args]` if Git is installed, or `git-chat [args]` if not.

If you're using git-chat for messaging with your savvy friends, you're an awesome person. But please do keep in mind that git-chat should not be used for secure and tamper proof communication, because it suffers from potential tampering (lost messages, erased history, etc.). Please use at your own risk.

## Compile
This project was designed to be built using cmake. We recommend version 2.8.11.

### Installing Dependencies
#### libgit2
This project depends on libgit2. To install it, just clone the libgit2 project from GitHub into the `extern/` directory at the root of the project, like so:

```
cd extern/
git clone git@github.com:libgit2/libgit2.git
```

You must also have OpenSSL and libSSH2 installed for libgit2 to build correctly. If you don't have them installed, the easiest way is to install using homebrew:

```
brew install openssl
brew install libssh2
```

libgit2 will automatically look for openssl and libssh2 at build time using `pkg-config`. If neither are installed in a standard location or cmake has trouble finding it, you can set these environment variables before running cmake. If using cmake through JetBrains CLion, refer to [this Stack Overflow answer](https://stackoverflow.com/a/38874446) for adding cmake environment variables:

```
CMAKE_PREFIX_PATH=/usr/local/
PKG_CONFIG_PATH=/usr/local/opt/openssl/lib/pkgconfig
OPENSSL_ROOT_DIR=/usr/local/opt/openssl/
OPENSSL_INCLUDE_DIR=/usr/local/opt/openssl/include/
```

#### GPGME
This project also depends on GnuPG Made Easy (GPGME). You can install the latest version [here](ftp://ftp.gnupg.org/gcrypt/gpgme/). Extract the sources into the project `extern/` directory at the root of the project.

To build GPGME, you also need to install [libgpg-error](ftp://ftp.gnupg.org/gcrypt/libgpg-error/) (>= 1.24) and [Libassuan](ftp://ftp.gnupg.org/gcrypt/libassuan/) (>= 2.4.2).

### Build Project
Once complete, the project should build using the following:

```
mkdir build
cd build
cmake ..
cmake --build .
```
