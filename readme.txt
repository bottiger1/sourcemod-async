We still must compile multiple libraries because the ubuntu/debian version either don't have static linking or they break when you try to static link them because they don't have fPIC...

But otherwise some of the libraries from Ubuntu 22.04 will work.

====Compiling libuv
Download from https://github.com/libuv/libuv
you must run cmake with this special flag to force all files to compile with fpic, or else linking will fail with the extension.
cmake -DCMAKE_C_FLAGS="-fPIC" .

copy the include and .a file to the libs folder

====Compiling zlib
Get the source code near the middle of the page here https://www.zlib.net/
Run this before ./configure and make
export CFLAGS="-fPIC"

copy .a file to the libs folder

==== Compiling nghttp2
download nghttp2 configure and install
https://github.com/nghttp2/nghttp2/releases
cmake .

64bit
./configure --enable-lib-only --disable-shared --enable-static

32bit
./configure --host=i686-pc-linux-gnu CC=gcc-4.8 CFLAGS=-m32
sudo make install

== compiling brotli

32bit only
sudo apt install -y libtool automake
git clone https://github.com/bagder/libbrotli.git
git submodule update --init --recursive
./autogen.sh
./configure --host=i686-pc-linux-gnu CC=gcc-4.8 CFLAGS=-m32


====Compiling curl
Add line to /etc/apt/source.list if missing
deb-src http://us.archive.ubuntu.com/ubuntu/ precise main restricted

32 bit
sudo apt-get -y install libc-ares-dev:i386 libssl-dev:i386 zlib1g-dev:i386 libuv-dev:i386 libcurl4-openssl-dev:i386 libbrotli-dev:i386 gcc-multilib

64 bit
sudo apt-get -y install libc-ares-dev libssl-dev libbrotli-dev


download curl configure and install
http://curl.haxx.se/download.html

32 bit
./configure --host=i686-pc-linux-gnu CC=gcc-4.8 CFLAGS=-m32 --disable-shared --enable-ares --with-openssl --with-zlib --without-libidn2 --disable-ldap --disable-ftp --disable-file --disable-dict --disable-telnet --disable-tftp --disable-rtsp --disable-pop3 --disable-imap --disable-smtp --disable-gopher --disable-rtmp --disable-smb --with-nghttp2 --with-brotli

64 bit
./configure --disable-shared --enable-ares --with-openssl --with-zlib --without-libidn2 --disable-telnet --disable-ldap --disable-ftp --disable-file --disable-dict --disable-tftp --disable-rtsp --disable-pop3 --disable-imap --disable-smtp --disable-gopher --disable-rtmp --disable-smb --disable-mqtt --without-zstd --without-ntlm --with-nghttp2

sudo make install