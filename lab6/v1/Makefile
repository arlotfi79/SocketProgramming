all: approutes approutec

approutes: approutes.o socket_utils.o utils.o constants.h
	@echo "Linking files and creating the client executable"
	gcc approutes.o socket_utils.o utils.o -o approutes.bin

approutec: approutec.o socket_utils.o utils.o constants.h
	@echo "Linking files and creating the server executable"
	gcc approutec.o utils.o socket_utils.o -o approutec.bin

approutes.o: approutes.c socket_utils.h utils.h constants.h
	@echo "Compiling the client tunnel file"
	gcc -c approutes.c

approutec.o: approutec.c socket_utils.h utils.h constants.h
	@echo "Compiling the client tunnel file"
	gcc -c approutec.c

socket_utils.o: socket_utils.c socket_utils.h
	@echo "Compiling the socket util files"
	gcc -c socket_utils.c

utils.o: utils.c utils.h
	@echo "Compiling the util files"
	gcc -c utils.c

clean:
	@echo "Removing everything but the source files"
	rm approutes.o approutec.o socket_utils.o utils.o approutec.bin approutes.bin