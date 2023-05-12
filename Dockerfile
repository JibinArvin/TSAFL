FROM ubuntu:22.04
RUN cp /etc/apt/sources.list /etc/apt/sources.list.bak
RUN sed -i s@/security.ubuntu.com/@/mirrors.aliyun.com/@g /etc/apt/sources.list
RUN sed -i s@/archive.ubuntu.com/@/mirrors.aliyun.com/@g /etc/apt/sources.list
RUN apt-get clean
RUN apt-get update --fix-missing

RUN apt-get install -y wget python3 clang-14 python3-pip git unzip zsh python-is-python3 python2 build-essential apt-utils tmux cmake libtool libtool-bin automake autoconf autotools-dev m4 autopoint libboost-dev help2man gnulib bison flex texinfo zlib1g-dev libexpat1-dev libfreetype6 dos2unix libfreetype6-dev libbz2-dev liblzo2-dev libtinfo-dev libssl-dev pkg-config libswscale-dev libarchive-dev liblzma-dev liblz4-dev doxygen vim intltool gcc-multilib libxml2 libxml2-dev sudo --fix-missing

RUN mkdir -p /workdir/tsafl

WORKDIR /workdir/tsafl
COPY . /workdir/tsafl

ENV PATH "/workdir/tsafl/clang+llvm/bin:$PATH"
ENV LD_LIBRARY_PATH "/workdir/tsafl/clang+llvm/lib:$LD_LIBRARY_PATH"
ENV ROOT_DIR "/workdir/tsafl/"

# Recommand user install wllvm by use-self. Before use wllvm, Plz read it's readme.txt first.
# RUN sudo pip install -e /workdir/tsafl/tool/wllvm/
RUN sudo pip install numpy
RUN sudo pip3 install numpy
RUN sudo pip3 install sysv_ipc 
RUN sudo pip3 install networkx 
RUN sudo pip3 install pydot
RUN sudo pip3 install wllvm

RUN mkdir /usr/local/share/aclocal/
RUN cp -rf /usr/share/aclocal/* /usr/local/share/aclocal/

RUN find /workdir/tsafl -name * | xargs dos2unix

# RUN tool/install_llvm.sh
# RUN tool/install_SVF.sh
# RUN tool/install_staticAnalysis.sh