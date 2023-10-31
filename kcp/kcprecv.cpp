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
    struct event_base *base;
    int total;
    int net_total;
	IUINT32 current;
	IUINT32 nextupdate;
} Handle;

Handle handle;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

// KCP回调函数，用于处理接收到的数据
int kcp_output(const char *buf, int len, ikcpcb *kcp, void *user) {
    Handle *handle = (Handle *)user;
    int n = sendto(handle->sockfd, buf, len, 0, (struct sockaddr*)&handle->addr, sizeof(handle->addr));
    // fprintf(stderr, "> kcp output callback %d send %d\n", len, n);
    return 0;
}

void timer_callback(evutil_socket_t fd, short events, void *arg) {
    Handle *handle = (Handle *)arg;

    pthread_mutex_lock(&lock);
	
	IUINT32 current = iclock();
	if (current >= handle->nextupdate) {
    	ikcp_update(handle->kcp, current);
		handle->nextupdate = ikcp_check(handle->kcp, current);
	}

    int peeksize = 0;
    while(peeksize = ikcp_peeksize(handle->kcp), peeksize > 0) {
        // 从KCP中接收数据
        char recvBuffer[BUFFER_SIZE];
        int recvlen = 0;
	while(recvlen = ikcp_recv(handle->kcp, recvBuffer, BUFFER_SIZE), recvlen > 0) {
            handle->total += recvlen;
            fprintf(stderr, "kcp recv %d peak %d total kcp %d net %d\r", recvlen, peeksize, handle->total, handle->net_total);
	}
    }
    pthread_mutex_unlock(&lock);
}


void *receive_handler(void *arg) {
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
        if (n < 0) {
            perror("recvfrom");
            exit(EXIT_FAILURE);
        } else if (n == 0) {
            printf("Peer closed the connection\n");
            break;
        } else {
			pthread_mutex_lock(&lock);
			handle->net_total += n;
			memcpy(&handle->addr, &addr, sizeof(addr));
        	int input = ikcp_input(handle->kcp, buffer, n);
			pthread_mutex_unlock(&lock);

			// fprintf(stderr, "< recving %d input %d\n", n, input);
        }
    }
    close(sockfd);
    return NULL;
}

int main() {
    // 创建文件
    FILE *file = fopen(FILENAME, "wb");
    if (!file) {
        perror("Failed to open file");
        return 1;
    }

    // 创建UDP套接字
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	handle.sockfd = sockfd;
    // evutil_make_socket_nonblocking(sockfd);
    
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(12345);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    // 直接使用bind系统调用绑定
    int ret = bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    fprintf(stderr, "bind %d\n", ret);

    // 创建libevent上下文
    struct event_base *base = event_base_new();
    handle.base = base;
    
    struct event *timer_event = event_new(base, -1, EV_PERSIST, timer_callback, &handle);
    struct timeval timer_interval = {0, 10000};
    event_add(timer_event, &timer_interval);

    // 创建KCP实例
    handle.kcp = ikcp_create(0x11223344, (void *)&handle);
    handle.kcp->output = kcp_output;
    handle.kcp->stream = 1;
    handle.nextupdate = iclock();

    // 设置KCP参数
    ikcp_wndsize(handle.kcp, 32, 32);
    ikcp_nodelay(handle.kcp, 0, 40, 0, 0);
    //ikcp_nodelay(handle.kcp, 1, 10, 2, 1);
    ikcp_setmtu(handle.kcp, 1200);

    pthread_t thread;
    ret = pthread_create(&thread, NULL, receive_handler, &handle);

    event_base_dispatch(base);

    pthread_join(thread, NULL); 

    // 关闭KCP和libevent
    // ikcp_release(kcp);
    event_base_free(base);

    // 关闭文件和套接字
    fclose(file);
    close(sockfd);

    return 0;
}
