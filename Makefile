CFLAGS=-ansi -Wpedantic -Wall 


all: Client.exe Server.exe

Client.exe: Client.o DBGpthread.o
	gcc ${CFLAGS} -o Client.exe Client.o DBGpthread.o -lpthread

Server.exe: Server.o DBGpthread.o Queue.o
	gcc ${CFLAGS} -o Server.exe Server.o DBGpthread.o  Queue.o -lpthread

Client.o: Client.c myfunction.h printerror.h DBGpthread.h
	gcc -c Client.c

Server.o: Server.c myfunction.h DBGpthread.h printerror.h Queue.h
	gcc -c Server.c

Queue.o: Queue.h
	gcc -c ${CFLAGS} Queue.c

DBGpthread.o: DBGpthread.h
	gcc -c ${CFLAGS} DBGpthread.c

.PHONY: clean

clean:
	rm -f Client.exe Server.exe *.o