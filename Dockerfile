FROM debian:11 AS builder
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    python3-pip python3-setuptools python3-wheel ninja-build \
    build-essential flex bison git cmake libsctp-dev \
    libgnutls28-dev libgcrypt-dev libssl-dev libmongoc-dev \
    libbson-dev libyaml-dev libnghttp2-dev libmicrohttpd-dev \
    libcurl4-gnutls-dev libtins-dev libtalloc-dev meson \
    libidn11-dev && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /open5gs
RUN git clone --depth 1 --branch v2.7.0 \
    https://github.com/open5gs/open5gs.git .

COPY patches/xor.c lib/crypt/xor.c
COPY patches/xor.h lib/crypt/xor.h
COPY patches/0001-xor-aka-patch.patch /patches/

RUN git apply /patches/0001-xor-aka-patch.patch

RUN meson build --prefix=/opt/open5gs && \
    ninja -C build && \
    ninja -C build install

FROM docker.io/gradiant/open5gs:2.7.0
COPY --from=builder /opt/open5gs /opt/open5gs
