FROM rockylinux/rockylinux:10

RUN dnf -y group install 'Development Tools'
RUN dnf -y install dnf-plugins-core sudo
RUN dnf config-manager --set-enabled crb
RUN dnf -y install epel-release

RUN useradd -s /bin/bash -m rpm
RUN echo >> /etc/sudoers
RUN echo "rpm ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

USER rpm
WORKDIR /home/rpm
