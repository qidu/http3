# apt install gcc-arm-linux-gnueabi
# apt install g++-arm-linux-gnueabi
gcc -g -I./kcp2 kcpsend.cpp kcp2/ikcp.c -levent -o send
gcc -g -I./kcp2 kcprecv.cpp kcp2/ikcp.c -levent -o recv
if [ -z /usr/bin/arm-linux-gnueabi-gcc ]
then
    /usr/bin/arm-linux-gnueabi-g++ -I./kcp2 kcprecv.cpp kcp2/ikcp.c -levent -o recv_kcparm
    /usr/bin/arm-linux-gnueabi-g++ -I./kcp2 kcpsend.cpp kcp2/ikcp.c -levent -o send_kcparm
fi
