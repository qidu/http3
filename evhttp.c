// gcc evhttp.c -levent -levent_openssl -lssl

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/keyvalq_struct.h>
#include <event2/bufferevent.h>
#include <event2/bufferevent_ssl.h>

struct request_handler {
    struct event_base * base;
    struct evhttp_connection * req_conn;
    SSL_CTX* ssl_ctx;
};

typedef struct request_handler request_handler_t;
typedef void(* req_callback_fn)(int, request_handler_t *, void *);

int request_send(request_handler_t* rh, const char* url);

static request_handler_t *s_handler = NULL;

static void sighandler(int signal)
{
	(void)signal;
	if(s_handler != NULL && s_handler->base != NULL)
		event_base_loopbreak(s_handler->base);
}

static void response_handle(struct evhttp_request * req, void * pvoid)
{
    size_t len;
	unsigned char * body_data;
	struct evbuffer * input_buffer;
	request_handler_t * rh = (request_handler_t *)pvoid;
    if(req == NULL) {
        return;
    }
    int state = evhttp_request_get_response_code(req);
    fprintf(stderr, "%d\n", state);

    input_buffer = evhttp_request_get_input_buffer(req);
	len = evbuffer_get_length(input_buffer);
	body_data = evbuffer_pullup(input_buffer, len);

	struct evkeyvalq * headers = evhttp_request_get_input_headers(req);
    struct evkeyval* h = headers->tqh_first;
    for (; h; h = h->next.tqe_next) {
        fprintf(stderr, "%s: %s\n", h->key, h->value);
    }
	const char *hd = evhttp_find_header(headers, "Location");
    if (hd) {
        fprintf(stderr, "Redirecting to: %s\n", hd);
        // evhttp_request_free(req);
        request_send(rh, hd);
        return;
    }
    fprintf(stdout, "%s\n", body_data);

    event_base_loopbreak(rh->base);
}

int request_send(request_handler_t* rh, const char* url)
{
    struct evhttp_request * req;
    struct evkeyvalq * headers;

    if (!rh || !rh->base || !url) {
        return -1;
    }

    struct evhttp_uri* uri = evhttp_uri_parse(url);
    const char* host = evhttp_uri_get_host(uri);
    int port = evhttp_uri_get_port(uri);
    const char *schema = evhttp_uri_get_scheme(uri);
    const char* path = evhttp_uri_get_path(uri);
    fprintf(stderr, "\nrequesting [%s %s %d %s]\n\n", schema, host, port, path);

    if (strncmp(schema, "https", 5) == 0) {
        rh->ssl_ctx = SSL_CTX_new(SSLv23_client_method());
        SSL_CTX_set_options(rh->ssl_ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION);
        struct bufferevent* bev = bufferevent_openssl_socket_new(rh->base, -1, SSL_new(rh->ssl_ctx), BUFFEREVENT_SSL_CONNECTING, BEV_OPT_CLOSE_ON_FREE);
        port = (port == -1) ? 443 : port;
        rh->req_conn = evhttp_connection_base_bufferevent_new(rh->base, NULL, bev, host, port);
    }
    else {
        port = (port == -1) ? 80 : port;
        rh->req_conn = evhttp_connection_base_new(rh->base, NULL, host, port);
    }
    req = evhttp_request_new(response_handle, rh);

    // buffer = evhttp_request_get_output_buffer(req);
    // evbuffer_add(buffer, body, body_len); // EVHTTP_REQ_POST body

    headers = evhttp_request_get_output_headers(req);
    evhttp_add_header(headers, "Host", host);
    evhttp_connection_set_timeout(rh->req_conn, 2);
    evhttp_make_request(rh->req_conn, req, EVHTTP_REQ_GET, path);
    return 0;
}

void timeout_callback(evutil_socket_t fd, short events, void* arg)
{
}

int main(int argc, char *argv[])
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = sighandler;
	if(sigaction(SIGINT, &sa, NULL) < 0) {
		perror("sigaction");
	}
    printf("Using libevent %s\n", event_get_version());
    if(LIBEVENT_VERSION_NUMBER != event_get_version_number()) {
		fprintf(stderr, "WARNING build using libevent %s", LIBEVENT_VERSION);
	}

    if (argc > 2) {
#ifdef DEBUG
	    event_enable_debug_mode();
#elif LIBEVENT_VERSION_NUMBER >= 0x02010100
	    event_enable_debug_logging(EVENT_DBG_ALL);	/* Libevent 2.1.1 */
#endif /* LIBEVENT_VERSION_NUMBER >= 0x02010100 */
    }

    if (argc < 2) {
		fprintf(stderr, "pls input url\n");
    }

    s_handler = (request_handler_t *)malloc(sizeof(request_handler_t));
    s_handler->base = event_base_new();
    if(s_handler->base == NULL) {
		fprintf(stderr, "event_base_new() failed\n");
		return 1;
	}

    struct event* timer_event = event_new(s_handler->base, -1, EV_TIMEOUT, timeout_callback, (void*)s_handler);
    struct timeval tv = {1, 0};
    // event_add(timer_event, &tv);

    request_send(s_handler, argv[1]);
    event_base_dispatch(s_handler->base);

    event_base_free(s_handler->base);
    SSL_CTX_free(s_handler->ssl_ctx);
    return 0;
}
