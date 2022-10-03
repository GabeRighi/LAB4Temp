server:
	gcc server.c -o server.out

client:
	gcc client.c -o client.out

s:
	./server.out

c:
	./client.out