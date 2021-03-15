FROM ubuntu:focal AS builder

RUN export DEBIAN_FRONTEND=noninteractive \
 && apt-get update \
 && apt-get install --no-install-recommends -y \
    git ca-certificates golang protobuf-compiler \
    libavahi-compat-libdnssd-dev libasound2-dev libasound2-plugins \
    cmake build-essential libssl-dev

WORKDIR /src

RUN git clone https://github.com/feelfreelinux/cspot.git cspot

WORKDIR /src/cspot

RUN git submodule update --init --recursive

WORKDIR /src/cspot/targets/cli/build

# cmake fix
RUN sed -i 's/VERSION 3.18/VERSION 3.16/' ../CMakeLists.txt
RUN cmake ..
RUN make

FROM ubuntu:focal

WORKDIR /app

COPY --from=builder \
     /usr/lib/x86_64-linux-gnu/libcrypto.so.1.1 \
     /usr/lib/x86_64-linux-gnu/libasound.so.2 \
     /usr/lib/x86_64-linux-gnu/libdns_sd.so.1 \
     /usr/lib/x86_64-linux-gnu/libavahi-common.so.3 \
     /usr/lib/x86_64-linux-gnu/libavahi-client.so.3 \
     /usr/lib/x86_64-linux-gnu/libdbus-1.so.3 \
     /usr/lib/x86_64-linux-gnu/
COPY --from=builder /src/cspot/targets/cli/build/cspotcli .

RUN export DEBIAN_FRONTEND=noninteractive \
 && apt-get update \
 && apt-get install --no-install-recommends -y \
    libasound2-plugins \
 && rm -rf /var/lib/apt/lists/*

# https://github.com/TheBiggerGuy/docker-pulseaudio-example/blob/master/Dockerfile
ENV UNAME cspot

RUN export UNAME=$UNAME UID=1000 GID=1000 && \
    mkdir -p "/home/${UNAME}" && \
    echo "${UNAME}:x:${UID}:${GID}:${UNAME} User,,,:/home/${UNAME}:/bin/bash" >> /etc/passwd && \
    echo "${UNAME}:x:${UID}:" >> /etc/group && \
    mkdir -p /etc/sudoers.d && \
    echo "${UNAME} ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers.d/${UNAME} && \
    chmod 0440 /etc/sudoers.d/${UNAME} && \
    chown ${UID}:${GID} -R /home/${UNAME} && \
    gpasswd -a ${UNAME} audio

COPY pulse-client.conf /etc/pulse/client.conf

USER $UNAME
ENV HOME /home/$UNAME

CMD ["/app/cspotcli"]
