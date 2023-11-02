function send() {
    { time ./send 100.100.71.186; } 2>> result-send.log
}

while(true); 
    do send;
    nc -N -l 12345 ;
    tail result-send.log | grep "real"
    sleep 5; 
done
