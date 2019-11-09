FROM centos:latest

RUN yum -y update

RUN yum -y groupinstall 'Development Tools'
RUN yum -y install dnf-utils
RUN yum -y install sudo

RUN dnf -y install https://dl.fedoraproject.org/pub/epel/epel-release-latest-8.noarch.rpm

RUN yum config-manager --set-enabled PowerTools

RUN yum -y update

RUN useradd -s /bin/bash -m rpm
RUN echo >> /etc/sudoers
RUN echo "rpm ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

USER rpm
WORKDIR /home/rpm
