FROM debian:11-slim
RUN apt-get update -y && \
    apt-get upgrade -y && \
    apt-get install -y build-essential wget git python3 && \
    wget -c "https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-rm/7-2018q2/gcc-arm-none-eabi-7-2018-q2-update-linux.tar.bz2" -O - \
    | tar -xj && \
    mv gcc-arm-none-eabi-7-2018-q2-update gcc-arm-none-eabi
ENV PATH="${PATH}:$PWD/gcc-arm-none-eabi/bin"
