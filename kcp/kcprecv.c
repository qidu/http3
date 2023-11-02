#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <event2/event.h>
#include <pthread.h>

#include "ikcp.h"

#define BUFFER_SIZE 200000
#define MAX_PACKET_SIZE 1500
#define FILENAME "/tmp/filename2"
#define DUMP_TO_FILE

/* get system time */
static inline void itimeofday(long *sec, long *usec)
{
    #if defined(__unix)
    struct timeval time;
    gettimeofday(&time, NULL);
    if (sec) *sec = time.tv_sec;
    if (usec) *usec = time.tv_usec;
    #else
    static long mode = 0, addsec = 0;
    BOOL retval;
    static IINT64 freq = 1;
    IINT64 qpc;
    if (mode == 0) {
        retval = QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
        freq = (freq == 0)? 1 : freq;
        retval = QueryPerformanceCounter((LARGE_INTEGER*)&qpc);
        addsec = (long)time(NULL);
        addsec = addsec - (long)((qpc / freq) & 0x7fffffff);
        mode = 1;
    }
    retval = QueryPerformanceCounter((LARGE_INTEGER*)&qpc);
    retval = retval * 2;
    if (sec) *sec = (long)(qpc / freq) + addsec;
    if (usec) *usec = (long)((qpc % freq) * 1000000 / freq);
    #endif
}

/* get clock in millisecond 64 */
static inline IINT64 iclock64(void)
{
    long s, u;
    IINT64 value;
    itimeofday(&s, &u);
    value = ((IINT64)s) * 1000 + (u / 1000);
    return value;
}

static inline IUINT32 iclock()
{
    return (IUINT32)(iclock64() & 0xfffffffful);
}

typedef struct {
    int sockfd;
    struct sockaddr_in addr;
    ikcpcb *kcp;
    int total;
    int net_total;
    FILE *file;
} Handle;

Handle handle;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

// KCP回调函数
int kcp_output(const char *buf, int len, ikcpcb *kcp, void *user) {
    Handle *handle = (Handle *)user;
    int n = sendto(handle->sockfd, buf, len, 0, (struct sockaddr*)&handle->addr, sizeof(handle->addr));
    return 0;
}

void do_receive(void *arg) {
    Handle *handle = (Handle *)arg;
    int sockfd = handle->sockfd;
    char buffer[MAX_PACKET_SIZE];
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    memset(&addr, 0, len);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(12345);
    addr.sin_addr.s_addr = INADDR_ANY;
    while (1) {
        int n = recvfrom(sockfd, buffer, MAX_PACKET_SIZE, 0, (struct sockaddr *)&addr, &len);
        if (n <= 0) {
            usleep(10000);
            continue;
        } else {
			handle->net_total += n;
			memcpy(&handle->addr, &addr, sizeof(addr));
        	int input = ikcp_input(handle->kcp, buffer, n);
            handle->kcp->current = iclock();
	        ikcp_flush(handle->kcp);
            
            int peeksize = 0;
            if (peeksize = ikcp_peeksize(handle->kcp), peeksize > 0) {
                // 从KCP中接收数据
                char recvBuffer[BUFFER_SIZE];
                int recvlen = 0;
                while(recvlen = ikcp_recv(handle->kcp, recvBuffer, BUFFER_SIZE), recvlen > 0) {
                    handle->total += recvlen;
                    if (recvlen >= 3 && strncmp(&recvBuffer[recvlen - 3], "bye", 3) == 0) {
                        fprintf(stderr, "kcp recv %d peak %d total kcp %d net %d    \n", recvlen, peeksize, handle->total, handle->net_total);
                        fprintf(stderr, "bye.\n");
                        return;
                    }
                    else {
#ifdef DUMP_TO_FILE
                        fwrite(recvBuffer, 1, recvlen, handle->file);
#endif
                        fprintf(stderr, "kcp recv %d peak %d total kcp %d net %d ...\r", recvlen, peeksize, handle->total, handle->net_total);
                    }
                }
            }
        }
    }
}

int set_dscp(int sock, int iptos) {
    iptos = (iptos << 2) & 0xFF;
    return setsockopt(sock, IPPROTO_IP, IP_TOS, &iptos, sizeof(iptos));
}

int main(int argc, char* argv[]) 
{
#ifdef DUMP_TO_FILE
    FILE *file = fopen(FILENAME, "wb");
    if (!file) {
        perror("Failed to open file");
        return 1;
    }
    handle.file = file;
#endif
    // 创建UDP套接字
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    set_dscp(sockfd, 46);
	handle.sockfd = sockfd;
    evutil_make_socket_nonblocking(sockfd);
    
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    if (argc > 1) {
        addr.sin_port = htons(atoi(argv[1]));
    }
    else {
        addr.sin_port = htons(12345);
    }
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int ret = bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    fprintf(stderr, "bind %s.\n", ret == 0 ? "success" : "failed");

    // 创建KCP实例
    handle.kcp = ikcp_create(0x11223344, 0, (void *)&handle);
    handle.kcp->output = kcp_output;
    handle.kcp->stream = 1;

    // 设置KCP参数
    ikcp_wndsize(handle.kcp, 2048, 2048);
    ikcp_nodelay(handle.kcp, 0, 40, 0, 0);
    // ikcp_nodelay(handle.kcp, 1, 10, 2, 1);
    ikcp_setmtu(handle.kcp, 1400);

    do_receive(&handle);

    ikcp_release(handle.kcp);
#ifdef DUMP_TO_FILE    
    fclose(file);
#endif
    close(sockfd);

    return 0;
}
