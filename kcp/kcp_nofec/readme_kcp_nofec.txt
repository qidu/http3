git remote -v
origin	git@github.com:qidu/libkcp.git (fetch)

WORKMODE=recv go build ../kcpserver.go 
./kcp_test_nofec 127.0.0.1 send

WORKMODE=send go build ../kcpserver.go 
./kcp_test_nofec 127.0.0.1 recv


FEC=1 WORKMODE=send go build ../kcpserver.go 
