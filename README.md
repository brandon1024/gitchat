# git-chat
`git-chat` is a Git-based command line messaging application. Version control systems were never built to be used as an messaging application, but Git is built such that it can be used as a chat application. So we set out to build this silly project.

The git-chat project is a silly experiment. The purpose of the project is to prove that Git can be used as a means of secure communication. Once installed, it can be run from the command line using `git chat [args]`.

If you're using git-chat for messaging with your savvy friends, you're an awesome person. But please do keep in mind that git-chat should not be used for secure and tamper proof communication, because it suffers from potential tampering (lost messages, erased history, etc.). Please use at your own risk.

## Compile
This project was designed to be built using cmake. We recommend version 2.8.11.

### Build and Run Project
The project should build using the following:

```
mkdir build
cd build
cmake ..
cmake --build .

./git-chat
```
