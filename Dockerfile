FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update
RUN apt-get install -y gcc g++ python libglib2.0-0 libglib2.0-dev libigraph0v5 libigraph0-dev cmake make xz-utils python-pyelftools \
      libc-dbg

COPY . /src

WORKDIR /src

ENV CC gcc
ENV CONTAINER ubuntu:20.04
ENV BUILDTYPE release
ENV RUSTPROFILE minimal

RUN ./setup build --clean --test

ENTRYPOINT ["shadow"]
