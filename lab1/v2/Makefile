all: remotecmdc remotecmds

remotecmdc: remotecmdc.o utils.o constants.h
	@echo "Linking files and creating the client executable"
	gcc remotecmdc.o utils.o -o remotecmdc.bin

remotecmds: remotecmds.o utils.o constants.h
	@echo "Linking files and creating the server executable"
	gcc remotecmds.o utils.o -o remotecmds.bin

remotecmdc.o: remotecmds.c utils.h constants.h
	@echo "Compiling the client file"
	gcc -c remotecmdc.c

remotecmds.o: remotecmds.c utils.h constants.h
	@echo "Compiling the server file"
	gcc -c remotecmds.c

utils.o: utils.c utils.h
	@echo "Compiling the util file"
	gcc -c utils.c

clean:
	@echo "Removing everything but the source files"
	rm remotecmdc.o remotecmds.o utils.o remotecmdc.bin remotecmds.bin cmdfifo
