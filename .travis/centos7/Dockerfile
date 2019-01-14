FROM centos:7

RUN yum -y groupinstall 'Development Tools'
RUN yum -y install centos-packager

RUN yum -y install sudo

RUN useradd -s /bin/bash -m rpm
RUN echo >> /etc/sudoers
RUN echo "rpm ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

USER rpm
WORKDIR /home/rpm
