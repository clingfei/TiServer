server.o: main.c
	cc -c main.c -o server.o
objects = server.o csapp.o
server:
	cc -o server $(objects) -lpthread
clean:
	rm *.o