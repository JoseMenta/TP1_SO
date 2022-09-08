rm resultado.csv
rm md5
rm slave
rm vista
rm /dev/shm/*

gcc -Wall -std=c99 md5.c -o md5 -pthread -lrt
gcc -Wall -std=c99 slave.c -o slave
gcc -Wall -std=c99 vista.c -o vista -pthread -lrt