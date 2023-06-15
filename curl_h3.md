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

```bash
cd nghttp3
sudo autoreconf -i
sudo ./configure --prefix=/usr/local/include/nghttp3 --enable-lib-only
sudo make -j4
sudo make install
cd ..

```

```bash
cd ngtcp2
sudo autoreconf -i
sudo ./configure PKG_CONFIG_PATH=/usr/local/include/openssl/lib64/pkgconfig:/usr/local/include/nghttp3/lib/pkgconfig LDFLAGS="-Wl,-rpath,/usr/local/include/openssl/lib64" --with-openssl --prefix=/usr/local/include/ngtcp2
make -j4
sudo make install
cd ..
```

```bash
echo $LD_LIBRARY_PATH
/usr/local/lib:/usr/local/include/openssl/lib64

cd curl
#./buildconf
sudo autoreconf -fi
LDFLAGS="-Wl,-rpath,/usr/local/include/openssl/lib64:/usr/local/include/nghttp3/lib:/usr/local/include/ngtcp2/lib" sudo -E ./configure --with-openssl=/usr/local/include/openssl --with-nghttp3=/usr/local/include/nghttp3 --with-ngtcp2=/usr/local/include/ngtcp2
sudo -E make -j4
```
