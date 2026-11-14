# syntax=docker/dockerfile:1.4
# EV Nova build kit: the one build host for every target (README.md). Build it from the repository root:
#   docker build --platform linux/amd64 --build-context inputs=input-files -t evnova-buildkit buildkit
# Wine runs the Metrowerks tools; unpack/ lays them out from the inputs folder. The image holds
# the Metrowerks tools: keep it private.
FROM --platform=linux/amd64 ubuntu:20.04
ENV DEBIAN_FRONTEND=noninteractive
RUN dpkg --add-architecture i386 && apt-get update && apt-get install -y --no-install-recommends \
        wine wine32 wine64 python3 p7zip-full unar unshield ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# a 32-bit Wine prefix, made once; no Mono or Gecko prompts
ENV WINEARCH=win32 WINEPREFIX=/root/.wine WINEDEBUG=-all WINEDLLOVERRIDES="mscoree,mshtml="
RUN wineboot --init && wineserver -w

# the Metrowerks tools, Apple's QuickTime library, unpacked from the inputs folder (unpack/)
COPY unpack/ /src/unpack/
RUN --mount=type=bind,from=inputs,target=/inputs python3 /src/unpack/unpack.py /inputs /opt/evnova

WORKDIR /repo
CMD ["python3", "/repo/buildkit/build.py", "all"]
