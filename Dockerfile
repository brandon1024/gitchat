FROM ubuntu:latest

# Hacking on git-chat with Docker
#
# Since Valgrind is difficult (impossible?) get work working on MacOS, this simple Docker image
# should be enough to run all necessary tests. It is assumed you already have an understanding
# of docker and how to use it.
#
# Build the image:
# docker build --tag=gitchat .
#
# Create and run git-chat from inside a docker container:
# $ docker run -it --volume $PWD:/git-chat gitchat
# root@41251d140930:/# cd /git-chat
# root@41251d140930:/git-chat/# mkdir build && cd build
# root@41251d140930:/git-chat/build/# cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..
# root@41251d140930:/git-chat/build/# make install
# root@41251d140930:/git-chat/build/# git-chat --version
#
# Run and attach to a stopped container:
# $ docker start -ai 41251d140930
#
# For details on running tests and valgrind, see README.md and integration-runner.sh

MAINTAINER Brandon Richardson <brandon1024.br@gmail.com>

RUN apt-get update && apt-get install -y \
	cmake \
	curl \
	gcc \
	git \
	gnupg2 \
	libgpgme-dev \
	man-db \
	ssh \
	valgrind \
	vim \
	wget

CMD ["/bin/bash"]