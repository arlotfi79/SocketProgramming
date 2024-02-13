all: pingc pings

pingc: pingc.o message_utils.o socket_utils.o constants.h
	@echo "Linking files and creating the client executable"
	gcc pingc.o message_utils.o socket_utils.o -o pingc.bin

pings: pings.o message_utils.o socket_utils.o constants.h
	@echo "Linking files and creating the server executable"
	gcc pings.o message_utils.o socket_utils.o -o pings.bin

pingc.o: pingc.c message_utils.h socket_utils.h constants.h
	@echo "Compiling the client file"
	gcc -c pingc.c

pings.o: pings.c message_utils.h socket_utils.h constants.h
	@echo "Compiling the server file"
	gcc -c pings.c

message_utils.o: message_utils.c message_utils.h
	@echo "Compiling the util file"
	gcc -c message_utils.c

socket_utils.o: socket_utils.c socket_utils.h
	@echo "Compiling the util file"
	gcc -c socket_utils.c

clean:
	@echo "Removing everything but the source files"
	rm pings.o pingc.o message_utils.o socket_utils.o pingc.bin pings.bin