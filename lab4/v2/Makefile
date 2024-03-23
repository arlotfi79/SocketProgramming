all: tunnels tunnelc

tunnels: tunnels.o socket_utils.o utils.o constants.h
	@echo "Linking files and creating the client executable"
	gcc tunnels.o socket_utils.o utils.o -o tunnels.bin

tunnelc: tunnelc.o socket_utils.o utils.o constants.h
	@echo "Linking files and creating the server executable"
	gcc tunnelc.o utils.o socket_utils.o -o tunnelc.bin

tunnels.o: tunnels.c socket_utils.h utils.h constants.h
	@echo "Compiling the client tunnel file"
	gcc -c tunnels.c

tunnelc.o: tunnelc.c socket_utils.h utils.h constants.h
	@echo "Compiling the client tunnel file"
	gcc -c tunnelc.c

socket_utils.o: socket_utils.c socket_utils.h
	@echo "Compiling the socket util files"
	gcc -c socket_utils.c

utils.o: utils.c utils.h
	@echo "Compiling the util files"
	gcc -c utils.c

clean:
	@echo "Removing everything but the source files"
	rm tunnels.o tunnelc.o socket_utils.o utils.o tunnelc.bin tunnels.bin