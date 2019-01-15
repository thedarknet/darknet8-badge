FROM ubuntu:18.04

# Install build dependencies (and vim + picocom for editing/debugging)
RUN apt -qq update \
    && apt install -y gcc git wget make libncurses-dev flex bison gperf python python-serial \
                          cmake ninja-build ccache vim picocom python-pip python-setuptools \
       			python-cryptography python-future python-pyparsing libusb-1.0 \
    && apt clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# Get the ESP32 toolchain
ENV ESP_TCHAIN_BASEDIR /opt/local/espressif

#get compiler
RUN mkdir -p $ESP_TCHAIN_BASEDIR \
    && wget -O $ESP_TCHAIN_BASEDIR/esp32-toolchain.tar.gz \
            https://dl.espressif.com/dl/xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz \
    && tar -xzf $ESP_TCHAIN_BASEDIR/esp32-toolchain.tar.gz \
           -C $ESP_TCHAIN_BASEDIR/ \
    && rm $ESP_TCHAIN_BASEDIR/esp32-toolchain.tar.gz

ENV ESP_OPENOCD $ESP_TCHAIN_BASEDIR/openocd

#get debugger
RUN wget -O $ESP_TCHAIN_BASEDIR/esp32-openocd.tar.gz \
	https://github.com/espressif/openocd-esp32/releases/download/v0.10.0-esp32-20181105/openocd-esp32-linux64-0.10.0-esp32-20181105.tar.gz \
	&& tar -zxf $ESP_TCHAIN_BASEDIR/esp32-openocd.tar.gz -C $ESP_TCHAIN_BASEDIR/ \
	&& rm $ESP_TCHAIN_BASEDIR/esp32-openocd.tar.gz

# Setup IDF_PATH
RUN mkdir -p /esp
RUN cd /esp && git clone --recursive https://github.com/espressif/esp-idf.git 
ENV IDF_PATH /esp/esp-idf
RUN python -m pip install -r $IDF_PATH/requirements.txt

# Add the toolchain binaries to PATH
ENV PATH $ESP_TCHAIN_BASEDIR/xtensa-esp32-elf/bin:$ESP_TCHAIN_BASEDIR/openocd-esp32/bin:$IDF_PATH/tools:$PATH

# This is the directory where our project will show up
RUN mkdir -p /esp/project
WORKDIR /esp/project
ENTRYPOINT ["/bin/bash"]

