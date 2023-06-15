- 获取代码
```bash
git clone -b openssl-3.09+quic https://github.com/quictls/openssl.git
git branch -a
* openssl-3.0.9+quic
  remotes/origin/HEAD -> origin/openssl-3.0.9+quic
```

```bash
git clone https://github.com/ngtcp2/ngtcp2
git clone https://github.com/ngtcp2/nghttp3
```

```bash
git clone https://github.com/curl/curl
git branch -a
  master
* ngtcp2-current
  remotes/origin/HEAD -> origin/master
```

- 编译

编openssl
```bash
cd openssl
sudo ./config enable-tls1_3 --prefix=/usr/local/include/openssl
sudo make -j4
sudo make install_sw
cd ..

ll /usr/local/include/openssl/
total 24
drwxr-xr-x 6 root root 4096 Jun 15 13:51 ./
drwxr-xr-x 5 root root 4096 Jun 13 12:35 ../
drwxr-xr-x 2 root root 4096 Jun 15 13:48 bin/
drwxr-xr-x 3 root root 4096 Jun 13 11:07 include/
lrwxrwxrwx 1 root root    5 Jun 15 13:51 lib -> lib64/
drwxr-xr-x 5 root root 4096 Jun 15 13:47 lib64/
```

编nghttp3
```bash
cd nghttp3
sudo autoreconf -i
sudo ./configure --prefix=/usr/local/include/nghttp3 --enable-lib-only
sudo make -j4
sudo make install
cd ..

ll -h /usr/local/include/nghttp3/
total 20K
drwxr-xr-x 5 root root 4.0K Jun 13 12:35 ./
drwxr-xr-x 5 root root 4.0K Jun 13 12:35 ../
drwxr-xr-x 3 root root 4.0K Jun 13 12:35 include/
drwxr-xr-x 3 root root 4.0K Jun 15 11:10 lib/
drwxr-xr-x 3 root root 4.0K Jun 13 12:35 share/
```
编ngtcp2
```bash
cd ngtcp2
sudo autoreconf -i
sudo ./configure PKG_CONFIG_PATH=/usr/local/include/openssl/lib64/pkgconfig:/usr/local/include/nghttp3/lib/pkgconfig LDFLAGS="-Wl,-rpath,/usr/local/include/openssl/lib64" --with-openssl --prefix=/usr/local/include/ngtcp2
make -j4
sudo make install
cd ..

ll -h /usr/local/include/ngtcp2/
total 20K
drwxr-xr-x 5 root root 4.0K Jun 13 12:24 ./
drwxr-xr-x 5 root root 4.0K Jun 13 12:35 ../
drwxr-xr-x 4 root root 4.0K Jun 13 13:45 include/
drwxr-xr-x 3 root root 4.0K Jun 15 14:02 lib/
drwxr-xr-x 3 root root 4.0K Jun 13 12:24 share/

# 注意_openssl*相关的库
ll -h /usr/local/include/ngtcp2/lib/
total 4.1M
drwxr-xr-x 3 root root 4.0K Jun 15 14:02 ./
drwxr-xr-x 5 root root 4.0K Jun 13 12:24 ../
-rw-r--r-- 1 root root 2.5M Jun 15 14:02 libngtcp2.a
-rwxr-xr-x 1 root root  971 Jun 15 14:02 libngtcp2.la*
lrwxrwxrwx 1 root root   19 Jun 15 14:02 libngtcp2.so -> libngtcp2.so.13.0.0*
lrwxrwxrwx 1 root root   19 Jun 15 14:02 libngtcp2.so.13 -> libngtcp2.so.13.0.0*
-rwxr-xr-x 1 root root 1.3M Jun 15 14:02 libngtcp2.so.13.0.0*
-rw-r--r-- 1 root root 196K Jun 15 14:02 libngtcp2_crypto_openssl.a
-rwxr-xr-x 1 root root 1.2K Jun 15 14:02 libngtcp2_crypto_openssl.la*
lrwxrwxrwx 1 root root   33 Jun 15 14:02 libngtcp2_crypto_openssl.so -> libngtcp2_crypto_openssl.so.5.0.0*
lrwxrwxrwx 1 root root   33 Jun 15 14:02 libngtcp2_crypto_openssl.so.5 -> libngtcp2_crypto_openssl.so.5.0.0*
-rwxr-xr-x 1 root root 128K Jun 15 14:02 libngtcp2_crypto_openssl.so.5.0.0*
drwxr-xr-x 2 root root 4.0K Jun 15 14:02 pkgconfig/
```

编译libcurl 和 curl
```bash
echo $LD_LIBRARY_PATH
/usr/local/lib:/usr/local/include/openssl/lib64

cd curl
#./buildconf
sudo autoreconf -fi
LDFLAGS="-Wl,-rpath,/usr/local/include/openssl/lib64:/usr/local/include/nghttp3/lib:/usr/local/include/ngtcp2/lib" sudo -E ./configure --with-openssl=/usr/local/include/openssl --with-nghttp3=/usr/local/include/nghttp3 --with-ngtcp2=/usr/local/include/ngtcp2
sudo -E make -j4

# 安装库libcurl
cd lib
sudo make install

ldd lib/.libs/libcurl.so
        linux-vdso.so.1 (0x00007ffde0f67000)
        libnghttp3.so.7 => /usr/local/include/nghttp3/lib/libnghttp3.so.7 (0x00007f67dac73000)
        libngtcp2_crypto_openssl.so.5 => /usr/local/include/ngtcp2/lib/libngtcp2_crypto_openssl.so.5 (0x00007f67dac67000)
        libngtcp2.so.13 => /usr/local/include/ngtcp2/lib/libngtcp2.so.13 (0x00007f67dac1c000)
        libssl.so.81.3 => /usr/local/include/openssl/lib64/libssl.so.81.3 (0x00007f67dab6a000)
        libcrypto.so.81.3 => /usr/local/include/openssl/lib64/libcrypto.so.81.3 (0x00007f67da6ea000)
        libz.so.1 => /lib/x86_64-linux-gnu/libz.so.1 (0x00007f67da6c2000)
        libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007f67da49a000)
        /lib64/ld-linux-x86-64.so.2 (0x00007f67dad43000)

ldd src/.libs/curl
        linux-vdso.so.1 (0x00007ffd405cd000)
        libcurl.so.4 => /usr/local/lib/libcurl.so.4 (0x00007f7e0d3b8000)
        libz.so.1 => /lib/x86_64-linux-gnu/libz.so.1 (0x00007f7e0d392000)
        libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007f7e0d16a000)
        libnghttp3.so.7 => /usr/local/include/nghttp3/lib/libnghttp3.so.7 (0x00007f7e0d140000)
        libngtcp2_crypto_openssl.so.5 => /usr/local/include/ngtcp2/lib/libngtcp2_crypto_openssl.so.5 (0x00007f7e0d134000)
        libngtcp2.so.13 => /usr/local/include/ngtcp2/lib/libngtcp2.so.13 (0x00007f7e0d0e7000)
        libssl.so.81.3 => /usr/local/include/openssl/lib64/libssl.so.81.3 (0x00007f7e0d035000)
        libcrypto.so.81.3 => /usr/local/include/openssl/lib64/libcrypto.so.81.3 (0x00007f7e0cbb5000)
        /lib64/ld-linux-x86-64.so.2 (0x00007f7e0d4a5000)
        
```
