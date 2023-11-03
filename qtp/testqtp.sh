# create test file data
dd if=/dev/zero of=/tmp/filename bs=1M count=2

# test send from client
WORKMODE=recv ./ktpserver
./qtp_test 127.0.0.1 send

# test send from serv
WORKMODE=send ./ktpserver
./qtp_test 127.0.0.1 recv
