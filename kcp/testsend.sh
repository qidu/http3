function send() {
    { time ./send 100.100.71.186; } 2>> result-send.log
}

while(true); 
    do send;
    sleep 10; 
done
