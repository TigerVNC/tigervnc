FROM ubuntu:trusty

RUN apt-get update
RUN apt-get -y install packaging-dev equivs

RUN useradd -s /bin/bash -m deb
RUN echo >> /etc/sudoers
RUN echo "deb ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

USER deb
WORKDIR /home/deb
