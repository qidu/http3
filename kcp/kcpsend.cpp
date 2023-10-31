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

#define BUFFER_SIZE 20000 
#define PACKET_SIZE 1000

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
    struct event_base *base;
    struct bufferevent* bev;
    struct evbuffer* inputbuf;
    struct evbuffer* outputbuf;
    int total;
    IUINT32 nextupdate;
} Handle; 

Handle handle;

// KCP回调函数，用于发送数据
int kcp_output(const char *buf, int len, ikcpcb *kcp, void *user) 
{
    Handle *handle = (Handle *)user;
    bufferevent_write(handle->bev, buf, len);
    bufferevent_flush(handle->bev, EV_WRITE, BEV_FLUSH);
    size_t buffer_len = evbuffer_get_length(bufferevent_get_output(handle->bev));
//    fprintf(stderr, "> kcp output %d buffering %lu\n", len, buffer_len);
	handle->inbuffer -= len;
    return 0;
}

void connect_callback(struct bufferevent* bev, short events, void* ctx)
{
    Handle *handle = (Handle *)ctx;
    if (events & BEV_EVENT_CONNECTED)
    {
        printf("Connected .\n");
    }
    if (events & BEV_EVENT_ERROR)
    {
        printf("Error.\n");
    }
    if (events & BEV_EVENT_READING)
    {
        printf("READING.\n");
    }
    if (events & BEV_EVENT_WRITING)
    {
        printf("WRITING.\n");
    }
    if (events & BEV_EVENT_EOF)
    {
        printf("Eof.\n");
    }
    else
    {
        printf("Connection failed %d.\n", events);
        // event_base_loopbreak(bufferevent_get_base(bev));
    }
}

void read_callback(struct bufferevent* bev, void* ctx)
{
    Handle *handle = (Handle *)ctx;

    // 读取数据
    char buf[PACKET_SIZE];
    size_t len = 0;
    while ( len = bufferevent_read(handle->bev, buf, PACKET_SIZE), len > 0)
    {
        int input = ikcp_input(handle->kcp, buf, len);
    //    fprintf(stderr, "< net recv %ld kcp input %d\n", len, input);
    }
	// perror("reading");
}

// 写事件回调函数
void write_callback(struct bufferevent* bev, void* ctx) {
    // 从KCP发送数据
    Handle *handle = (Handle *)ctx;
	// perror("writing");
}

// 定时器回调函数，用于触发KCP更新
void timer_callback(evutil_socket_t fd, short events, void *arg)
{
    Handle *handle = (Handle *)arg;
	IUINT32 current = iclock();
	if (current >= handle->nextupdate) {
    	ikcp_update(handle->kcp, current);
		handle->nextupdate = ikcp_check(handle->kcp, current);
	}

	if (handle->inbuffer < 0) {
    	char buffer[BUFFER_SIZE];
    	handle->inbuffer = fread(buffer, 1, BUFFER_SIZE, handle->file);
		if (handle->inbuffer > 0) {
        	handle->total += handle->inbuffer;
        	fprintf(stderr, "@ read %d total %d\r", handle->inbuffer, handle->total);
        	ikcp_send(handle->kcp, buffer, handle->inbuffer);
        	ikcp_flush(handle->kcp);
		}
		else {
			fprintf(stderr, "read file error, quiting\n");
    		event_base_loopbreak(bufferevent_get_base(handle->bev));
		}
	}
}

int main(int argc, char* argv[]) 
{
    // 打开文件
    handle.file = fopen(FILENAME, "rb");
    if (!handle.file) {
        perror("Failed to open file");
        return 1;
    }
    
    
    // 创建UDP套接字
    handle.sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    evutil_make_socket_nonblocking(handle.sockfd);
    // 创建libevent上下文
    handle.base = event_base_new();
    handle.bev = bufferevent_socket_new(handle.base, handle.sockfd, BEV_OPT_CLOSE_ON_FREE);

    // 创建写事件
    bufferevent_setcb(handle.bev, read_callback, write_callback, NULL, &handle);
    bufferevent_setwatermark(handle.bev, EV_WRITE, 0, 10);

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(TARGET_PORT);
    inet_pton(AF_INET, (argc > 1? argv[1] : TARGET_IP), &(serv_addr.sin_addr));

    bufferevent_socket_connect(handle.bev, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    bufferevent_enable(handle.bev, EV_READ | EV_WRITE);

    // 创建定时器事件，每隔10毫秒触发一次
    struct event *timer_event = event_new(handle.base, -1, EV_PERSIST, timer_callback, &handle);
    struct timeval timer_interval = {0, 50000};
    event_add(timer_event, &timer_interval);
	
	// 创建KCP实例
    handle.kcp = ikcp_create(0x11223344, (void*)&handle);
    handle.kcp->output = kcp_output;
    handle.kcp->stream = 1;
    handle.nextupdate = iclock();

    // 设置KCP参数
    ikcp_wndsize(handle.kcp, 32, 32);
    ikcp_nodelay(handle.kcp, 0, 40, 0, 0);
    //ikcp_nodelay(handle.kcp, 1, 10, 2, 1);
    ikcp_setmtu(handle.kcp, 1200);

    // 读取文件数据
    char buffer[BUFFER_SIZE];
    if ((handle.inbuffer = fread(buffer, 1, BUFFER_SIZE, handle.file)) > 0) {
        handle.total += handle.inbuffer;
        fprintf(stderr, "@ read %d total %d\n", handle.inbuffer, handle.total);
        ikcp_send(handle.kcp, buffer, handle.inbuffer);
        ikcp_flush(handle.kcp);
    }
  
    event_base_dispatch(handle.base);

    // 发送结束标识
    fprintf(stderr, "finish send, waisnd\n");

    // 进入事件循环，直到所有数据发送完成
    int ret = 0;
    while (ret = ikcp_waitsnd(handle.kcp) > 0) {
        // fprintf(stderr, "waitsnd finish... %d\n", ret);
        event_base_dispatch(handle.base);
		sleep(1);
    }

    // 释放资源
    ikcp_release(handle.kcp);
    event_base_free(handle.base);
    event_free(timer_event);

    // 关闭文件和套接字
    fclose(handle.file);
    close(handle.sockfd);

    return 0;
}

