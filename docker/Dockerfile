FROM ubuntu:18.04

# Install build dependencies (and vim + picocom for editing/debugging)
RUN apt -qq update \
    && apt install -y gcc git wget make libncurses-dev flex bison gperf python python-serial curl \
                          cmake ninja-build ccache vim picocom python-pip python-setuptools \
       			python-cryptography python-future python-pyparsing libusb-1.0 telnet ccache dosfstools \
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
RUN mkdir -p /ccache
# fetch IDF and make writeable 
# ERROR: esp-idf must be cloned from git to work.
# RUN wget -O /esp/esp-idf.tar.gz \
#	https://github.com/espressif/esp-idf/archive/v3.1.2.tar.gz \
#	&& tar -zxf /esp/esp-idf.tar.gz -C /esp/  \
#	&& ln -s /esp/esp-idf-3.1.2 /esp/esp-idf \
#	&& chmod -R 775 /esp/esp-idf

RUN useradd -m -u 10000 -G plugdev,dialout,root -s /bin/bash dndev

#RUN su - dndev -c "cd /esp && git clone --branch v3.2 --depth 1 --recursive https://github.com/espressif/esp-idf.git && chmod -R 775 esp-idf"
RUN cd /esp && git clone --branch v3.2.2 --depth 1 --recursive https://github.com/espressif/esp-idf.git && chmod -R 775 esp-idf
ENV IDF_PATH /esp/esp-idf
RUN python -m pip install -r $IDF_PATH/requirements.txt

# Add the toolchain binaries to PATH
ENV PATH $ESP_TCHAIN_BASEDIR/xtensa-esp32-elf/bin:$ESP_TCHAIN_BASEDIR/openocd-esp32/bin:$IDF_PATH/tools:$PATH

RUN echo "#########\n# START Added by docker buid\n######" >> /home/dndev/.bashrc
RUN echo "export PATH=$ESP_TCHAIN_BASEDIR/xtensa-esp32-elf/bin:$ESP_TCHAIN_BASEDIR/openocd-esp32/bin:$IDF_PATH/tools:$PATH" >> /home/dndev/.bashrc
RUN echo "#########\n# END Added by docker buid\n######" >> /home/dndev/.bashrc
COPY --chown=dndev:dndev ./vimrc /home/dndev/.vimrc
# install vundle
RUN su - dndev -c "git clone https://github.com/VundleVim/Vundle.vim.git /home/dndev/.vim/bundle/Vundle.vim"
RUN su - dndev -c "vim +PluginInstall +qall"

COPY ./entry.sh /bin/entry.sh 
RUN chmod +x /bin/entry.sh
# This is the directory where our project will show up
RUN mkdir -p /esp/project
WORKDIR /esp/project
ENTRYPOINT ["/bin/entry.sh"]

