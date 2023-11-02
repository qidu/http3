#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/util.h>

#include "ikcp.h"

#define BUFFER_SIZE 100000
#define PACKET_SIZE 1400

#define FILENAME "/tmp/filename"
#define TARGET_IP "127.0.0.1"
#define TARGET_PORT 12345

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
    FILE* file;
    int sockfd;
    int sended;
    int inbuffer;
    ikcpcb *kcp;
    struct sockaddr_in server;
    struct event_base *base;
    int total;
} Handle; 

Handle handle;

// KCP回调函数，用于发送数据
int kcp_output(const char *buf, int len, ikcpcb *kcp, void *user) 
{
    Handle *handle = (Handle *)user;
    handle->inbuffer -= len;  // total len bigger than inbuffer

    int sent = 0;
    int packet_len = PACKET_SIZE;
    char buffer[PACKET_SIZE];
    while (len > 0) {
        packet_len = PACKET_SIZE;
        if (len < PACKET_SIZE) {
            packet_len = len;
        }
        memcpy(buffer, buf + sent, packet_len);
        sent += packet_len;
        sendto(handle->sockfd, buffer, packet_len, 0, (struct sockaddr *)&handle->server, sizeof(struct sockaddr_in));
        len -= packet_len;
    }

    return 0;
}

// 定时器回调函数，用于触发KCP更新
void timer_callback(evutil_socket_t fd, short events, void *arg)
{
    Handle *handle = (Handle *)arg;
	if (handle->inbuffer < 0) {
    	char buffer[BUFFER_SIZE];
    	handle->inbuffer = fread(buffer, 1, BUFFER_SIZE, handle->file);
		if (handle->inbuffer > 0) {
        	handle->total += handle->inbuffer;
        	fprintf(stderr, "@ read %d total %d ...\r", handle->inbuffer, handle->total);
        	ikcp_send(handle->kcp, buffer, handle->inbuffer);
		}
		else {
			fprintf(stderr, "@ read total %d\n", handle->total);
    		event_base_loopbreak(handle->base);
		}
	}
    handle->kcp->current = iclock();
    ikcp_flush(handle->kcp);
}

void read_callback(evutil_socket_t sockfd, short event, void *arg) 
{
    char buffer[BUFFER_SIZE];
    Handle *handle = (Handle *)arg;
    if (event & EV_READ) {
        socklen_t addr_len;
        ssize_t ret = recvfrom(handle->sockfd, buffer, sizeof(buffer), 1, (struct sockaddr *)&handle->server, &addr_len);
        if (ret > 0) {
            int input = ikcp_input(handle->kcp, buffer, ret);
        }
        handle->kcp->current = iclock();
        ikcp_flush(handle->kcp);
    }
    else {

    }
}

int set_dscp(int sock, int iptos) {
    iptos = (iptos << 2) & 0xFF;
    return setsockopt(sock, IPPROTO_IP, IP_TOS, &iptos, sizeof(iptos));
}

int main(int argc, char* argv[]) 
{
    // 打开文件
    handle.file = fopen(FILENAME, "rb");
    if (!handle.file) {
        fprintf(stderr, "Failed to open file %s", FILENAME);
        return 1;
    }
    
    
    // 创建UDP套接字
    handle.sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    set_dscp(handle.sockfd, 46);
    evutil_make_socket_nonblocking(handle.sockfd);
    // 创建libevent上下文
    handle.base = event_base_new();

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(TARGET_PORT);
    if (argc > 1) {
        inet_pton(AF_INET, argv[1], &(serv_addr.sin_addr));
    }
    else {
        inet_pton(AF_INET, TARGET_IP, &(serv_addr.sin_addr));
    }
    memcpy(&handle.server, &serv_addr, sizeof(struct sockaddr_in));

    struct event *ev = event_new(handle.base, handle.sockfd, EV_READ | EV_PERSIST, read_callback, &handle);
    event_add(ev, NULL);

    struct event *timer_event = event_new(handle.base, -1, EV_PERSIST, timer_callback, &handle);
    struct timeval timer_interval = {0, 10000};
    event_add(timer_event, &timer_interval);
	
    handle.kcp = ikcp_create(0x11223344, 0, (void*)&handle);
    handle.kcp->output = kcp_output;
    handle.kcp->stream = 1;

    ikcp_wndsize(handle.kcp, 512, 512);
    ikcp_nodelay(handle.kcp, 0, 40, 0, 0);
    //ikcp_nodelay(handle.kcp, 1, 10, 2, 1);
    ikcp_setmtu(handle.kcp, 1400);

    handle.inbuffer = -1;
  
    event_base_dispatch(handle.base);
    ikcp_send(handle.kcp, "bye", 3);
    ikcp_flush(handle.kcp);

    int ret = ikcp_waitsnd(handle.kcp);
    fprintf(stderr, "finish sending %d.\n", ret);

    
    // 释放资源
    ikcp_release(handle.kcp);
    event_base_free(handle.base);

    // 关闭文件和套接字
    fclose(handle.file);
    close(handle.sockfd);

    return 0;
}

