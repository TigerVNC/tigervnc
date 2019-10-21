FROM centos:latest

RUN echo 10.197.100.1 > /etc/resolv.conf

RUN yum -y groupinstall 'Development Tools'
RUN yum -y install dnf-utils --nogpgcheck
RUN yum -y install wget
RUN yum -y install sudo

RUN dnf -y install https://dl.fedoraproject.org/pub/epel/epel-release-latest-8.noarch.rpm

RUN yum config-manager --set-enabled PowerTools

RUN wget http://mirror.centos.org/centos/7/os/x86_64/Packages/xorg-x11-server-source-1.20.4-7.el7.noarch.rpm

RUN yum -y install xorg-x11-server-source-1.20.4-7.el7.noarch.rpm

RUN rm xorg-x11-server-source-1.20.4-7.el7.noarch.rpm

RUN yum -y update

RUN useradd -s /bin/bash -m rpm
RUN echo >> /etc/sudoers
RUN echo "rpm ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

USER rpm
WORKDIR /home/rpm
