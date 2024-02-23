all: remotecmdc remotecmds

remotecmdc: remotecmdc.o utils.o socket_utils.o constants.h
	@echo "Linking files and creating the client executable"
	gcc remotecmdc.o utils.o socket_utils.o -o remotecmdc.bin

remotecmds: remotecmds.o utils.o socket_utils.o constants.h
	@echo "Linking files and creating the server executable"
	gcc remotecmds.o utils.o socket_utils.o -o remotecmds.bin

remotecmdc.o: remotecmds.c utils.h socket_utils.h constants.h
	@echo "Compiling the client file"
	gcc -c remotecmdc.c

remotecmds.o: remotecmds.c utils.h socket_utils.h constants.h
	@echo "Compiling the server file"
	gcc -c remotecmds.c

socket_utils.o: socket_utils.c socket_utils.h
	@echo "Compiling the util file"
	gcc -c socket_utils.c

utils.o: utils.c utils.h constants.h
	@echo "Compiling the util file"
	gcc -c utils.c

clean:
	@echo "Removing everything but the source files"
	rm remotecmdc.o remotecmds.o utils.o socket_utils.o remotecmdc.bin remotecmds.bin
