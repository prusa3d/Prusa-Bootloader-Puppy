FROM debian:11-slim
RUN apt-get update -y && \
    apt-get upgrade -y && \
    apt-get install -y build-essential wget git python3 && \
    wget -c "https://developer.arm.com/-/media/Files/downloads/gnu/13.3.rel1/binrel/arm-gnu-toolchain-13.3.rel1-x86_64-arm-none-eabi.tar.xz" -O - \
    | tar -xJ && \
    mv arm-gnu-toolchain-13.3.rel1-x86_64-arm-none-eabi gcc-arm-none-eabi
ENV PATH="${PATH}:$PWD/gcc-arm-none-eabi/bin"
