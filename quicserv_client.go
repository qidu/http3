package main

import (
	"crypto/tls"
	"os"
	"io"
	"time"
	"log"
	"fmt"
	"net/http"

	"github.com/quic-go/quic-go/http3"
    "github.com/quic-go/quic-go/interop/utils"
	"github.com/quic-go/quic-go"
)

func main() {
    go server()
    client()
}

func client() {
	// 创建HTTP/3客户端传输
	transport := &http3.RoundTripper{
		TLSClientConfig: &tls.Config{
			InsecureSkipVerify: true, // 用于示例，实际使用中请配置正确的证书验证
		},
        QuicConfig: &quic.Config{
            Allow0RTT: true,
        },
        EnableDatagrams: true,
	}

	// 创建HTTP客户端
	client := &http.Client{
		Transport: transport,
        Timeout:   1 * time.Second,
	}

    for i := 0; i < 50; i++ {
        // 发送GET请求
        resp, err := client.Get("https://localhost:8088")
        if err != nil {
            log.Fatal(err)
        }
        defer resp.Body.Close()

        // 读取响应
        body, err := io.ReadAll(resp.Body)
        if err != nil {
            log.Fatal(err)
        }

        log.Println("[Client] Get Response:%v", string(body))
        time.Sleep(1 * time.Second)
    }
}

func server() {
    // 加载证书和私钥
	cert, err := tls.LoadX509KeyPair("cert.pem", "key.pem")
	if err != nil {
		log.Fatal(err)
	}
    // fmt.Println("%v", cert)

    keyLog, err := utils.GetSSLKeyLog() // export SSLKEYLOGFILE=./logkey.log
	if err != nil {
		fmt.Printf("Could not create key log: %s\n", err.Error())
		os.Exit(1)
	}
	if keyLog != nil {
		defer keyLog.Close()
	}
	// 创建TLS配置
	tlsConfig := &tls.Config{
		Certificates: []tls.Certificate{cert},
		NextProtos:   []string{http3.NextProtoH3},
        KeyLogWriter:       keyLog,
	}

    // 创建调试配置
	quicConfig := &quic.Config{
        Allow0RTT: true,
        DisablePathMTUDiscovery: true,
        MaxIdleTimeout: 4 * time.Second,
        Tracer: utils.NewQLOGConnectionTracer, // export QLOGDIR=.
	}
    log.Println("%v", quicConfig)

	// 创建HTTP/3服务器
	server := &http3.Server{
		Addr:      ":8088",
		Handler:   http.HandlerFunc(handleRequest),
		TLSConfig: tlsConfig,
		QuicConfig: quicConfig,
	}

    // fmt.Println("%v", server)
	// 启动HTTP/3服务器
	log.Println("listening to localhost:8088")
	err = server.ListenAndServe()
	if err != nil {
		log.Fatal(err)
	}
	log.Println("Server listening on localhost:8088")
}

func handleRequest(w http.ResponseWriter, r *http.Request) {
	// 处理请求
	log.Println("[Server] Received request:", r.URL.Path)
	w.Write([]byte("Hello, client!"))
}
