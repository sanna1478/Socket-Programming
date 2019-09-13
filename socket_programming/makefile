all: Client AWS Monitor ServerA ServerB

Client: client.c
	gcc -g -Wall -o client client.c

AWS: aws.c
	gcc -g -Wall -o aws aws.c

Monitor: monitor.c
	gcc -g -Wall -o monitor monitor.c

ServerA: serverA.c
	gcc -g -Wall -o serverA serverA.c

ServerB: serverB.c
	gcc -g -Wall -o serverB serverB.c -lm

.PHONY: aws
aws: AWS
	./aws

.PHONY: monitor
monitor: Monitor
	./monitor

.PHONY: serverA
serverA: ServerA
	./serverA

.PHONY: serverB
serverB: ServerB
	./serverB

clean:
	${RM} *.o client aws monitor serverA serverB
