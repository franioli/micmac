FROM ubuntu:22.04 

# Add metadata
LABEL description="MicMac Photogrammetry Software Container for Headless systems"

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive
ENV PYTHONUNBUFFERED=1
ENV MICMAC_DIR=/opt/micmac
ENV PATH=${MICMAC_DIR}/bin:$PATH

# Set working directory
WORKDIR /opt

#MicMac dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    make \
    cmake \ 
    git \
    proj-bin \
    exiv2 \
    exiftool \
    imagemagick \
    xorg \
    qtbase5-dev \
    qt5-qmake \
    btop \ 
    nano \
    bash-completion \
    python3 \
    python3-pip

#MicMac clone
RUN git clone https://github.com/micmacIGN/micmac.git

#MicMac build & compile
RUN cd micmac && \
    mkdir build && \
    cd build && \
    cmake ../ && make install -j16

#MicMac add environmental variable to executables
RUN echo "${MICMAC_DIR}/bin/:${PATH}"

# Configure bash environment
RUN \
    # Add aliases for python and pip 
    echo "# Bash Aliases" >> /root/.bashrc && \
    echo "alias python=python3" >> /root/.bashrc && \
    echo "alias pip=pip3" >> /root/.bashrc && \
    echo "" >> /root/.bashrc && \
    # Enable bash completion
    echo "# Enable bash completion" >> /root/.bashrc && \
    echo "if [ -f /etc/bash_completion ]; then . /etc/bash_completion; fi" >> /root/.bashrc && \
    echo "" >> /root/.bashrc

WORKDIR /workspace

# Set entrypoint
ENTRYPOINT ["/bin/bash"]