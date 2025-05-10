#1.
```
gcc -std=gnu11 -Wall -Wextra -o program main.c

./program
```
и в другой консоли(секунд 10 подождем)
```
echo "First message" > /tmp/exo-server
```

Получаем какой то такой вывод
```
FIFO created.
Received SIGALRM: still waiting for data...
First message
^CReceived SIGINT: finishing current read, then exiting
Stats: 1 messages, 14 bytes, 1 alarms
```

#2.
Из демона примерно тоже самое: (только вывод в файл)
```
FIFO created.
Received SIGALRM: still waiting for data...
1 message
Received SIGALRM: still waiting for data...
Received SIGINT: finishing current read, then exiting
Stats: 1 messages, 10 bytes, 2 alarms
```

Конечно нужно сделать что то типo:
```
ps -ef | grep './program'
kill -INT <pid>
```


#3.

Опять
```
./program
```

И в другой консоли
```
ps -ef | grep './program'
```
```
kill -TERM <pid>
```

Увидим что то похожее на:
```
FIFO created.
Received SIGALRM: still waiting for data...
Received SIGTERM: terminating immediately upon user request
Stats: 0 messages, 0 bytes, 1 alarms
```


#4 Сигналы.

```
root@4605219-pr60097:~/os_labs# ps -ef | grep './program'
root       52115   51371  0 01:22 pts/1    00:00:00 ./program
root       52135   52009  0 01:23 pts/0    00:00:00 grep --color=auto ./program
root@4605219-pr60097:~/os_labs# kill -QUIT 52115
root@4605219-pr60097:~/os_labs# kill -USR1 52115
root@4605219-pr60097:~/os_labs# kill -HUP 52115
```

Это в консоль.
```
FIFO created.
Received SIGALRM: still waiting for data...
Received SIGALRM: still waiting for data...
Received SIGALRM: still waiting for data...
Received SIGALRM: still waiting for data...
Received SIGALRM: still waiting for data...
Received SIGALRM: still waiting for data...
Received SIGALRM: still waiting for data...
Received SIGALRM: still waiting for data...
Stats: 0 messages, 0 bytes, 8 alarms
Received SIGALRM: still waiting for data...
```
