FROM debian:latest
RUN apt-get -qq update
RUN apt-get install -y libax25 ax25-apps ax25-tools libax25-dev build-essential automake libtool libncurses-dev
ENV HOME /home/linpac
RUN useradd -c "Linpac user" -d $HOME -m linpac



