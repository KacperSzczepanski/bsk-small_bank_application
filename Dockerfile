FROM ubuntu:18.04

MAINTAINER ks418474
COPY . /

RUN apt-get update
RUN apt update

RUN apt-get -y install gcc make acl libpam0g-dev sudo openssh-server
RUN apt install locales

RUN locale-gen en_US.UTF-8

RUN apt-get -y install python3
RUN apt-get -y install python3-pip

RUN pip3 install django
RUN pip3 install python-pam

ENV PYTHONUNBUFFERED=1

RUN echo "Match Group officers" >> /etc/ssh/sshd_config
RUN echo "	ForceCommand /officerDir/officerApp" >> /etc/ssh/sshd_config

CMD ./init.sh uzytkownicy.txt >/dev/null 2>/dev/null && python3 clientDir/bankapp/manage.py runserver 0.0.0.0:8080
