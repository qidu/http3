# apt install gcc-arm-linux-gnueabi
# apt install g++-arm-linux-gnueabi
# apt install gcc-aarch64-linux-gnu
gcc -g -I./kcp2 kcpsend.c kcp2/ikcp.c -levent -o send
gcc -g -I./kcp2 kcprecv.c kcp2/ikcp.c -levent -o recv
if [ -f /usr/bin/aarch64-linux-gnu-gcc ]  && [ -f libevent/.libs/libevent.a ]
then
# sudo -E ./configure --host=arm-linux-gnueabi --build=arm64 CC=/usr/bin/aarch64-linux-gnu-gcc
    /usr/bin/aarch64-linux-gnu-gcc -I./kcp2 kcprecv.c kcp2/ikcp.c libevent/.libs/libevent.a -o recv_kcparm
    /usr/bin/aarch64-linux-gnu-gcc -I./kcp2 kcpsend.c kcp2/ikcp.c libevent/.libs/libevent.a -o send_kcparm
fi
