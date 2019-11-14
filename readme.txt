These are some rough notes on what I needed to do to compile the extension on Ubuntu 18.04 x64.

There is a copy of libuv in the project directory because when I wrote this extension, libuv was difficult to compile and did not install in the right place to get automatically detected by g++. This may have changed now so maybe we will remove it in the future.

Compiling libuv
Download from https://github.com/libuv/libuv
python gyp_uv.py -Dtarget_arch=ia32 -f make
cd out
sudo apt install -y g++ g++-multilib
make BUILDTYPE=Release

copy the include and .a file to the libs folder

Compiling curl
Add line to /etc/apt/source.list if missing
deb-src http://us.archive.ubuntu.com/ubuntu/ precise main restricted
sudo apt-get -y install libc-ares-dev:i386 libssl-dev:i386 zlib1g-dev:i386 libboost-dev libuv-dev:i386 libcurl4-openssl-dev:i386 libbrotli-dev:i386 gcc-multilib

download nghttp2 configure and install
https://github.com/nghttp2/nghttp2/releases

./configure --host=i686-pc-linux-gnu CC=gcc-4.8 CFLAGS=-m32
sudo make install

brotli

sudo apt install -y libtool automake
git clone https://github.com/bagder/libbrotli.git
git submodule update --init --recursive
./autogen.sh
./configure --host=i686-pc-linux-gnu CC=gcc-4.8 CFLAGS=-m32

download curl configure and install
http://curl.haxx.se/download.html

./configure --host=i686-pc-linux-gnu CC=gcc-4.8 CFLAGS=-m32 --disable-shared --enable-ares --with-openssl --with-zlib --without-libidn2 --disable-ldap --disable-ftp --disable-file --disable-dict --disable-telnet --disable-tftp --disable-rtsp --disable-pop3 --disable-imap --disable-smtp --disable-gopher --disable-rtmp --disable-smb --with-nghttp2 --with-brotli
sudo make install
