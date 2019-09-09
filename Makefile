all: bankingServer.c bankingClient.c strhelper.c iohelper.c
	make clean;
	gcc -c strhelper.c;
	gcc -c iohelper.c;
	gcc -c bankingServer.c;
	gcc -c bankingClient.c;
	gcc strhelper.o iohelper.o bankingServer.o -lm -pthread -o bankingServer;
	gcc strhelper.o iohelper.o bankingClient.o -lm -pthread -o bankingClient;
	make clean-obj;

clean:
	rm -f bankingServer;
	rm -f bankingClient;
	make clean-obj;

clean-obj:
	rm -f strhelper.o;
	rm -f iohelper.o
	rm -f bankingServer.o;
	rm -f bankingClient.o;
