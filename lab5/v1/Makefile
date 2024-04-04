all: audiostreams audiostreamc

audiostreamc: audiostreamc.o socket_utils.o congestion_control.o ../lib/constants.h
	@echo "Linking files and creating the client executable"
	gcc audiostreamc.o congestion_control.o socket_utils.o -o audiostreamc.bin

audiostreams: audiostreams.o socket_utils.o congestion_control.o ../lib/constants.h
	@echo "Linking files and creating the server executable"
	gcc audiostreams.o congestion_control.o socket_utils.o -o audiostreams.bin

audiostreamc.o: audiostreamc.c ../lib/socket_utils.h ../lib/socket_utils.h ../lib/constants.h
	@echo "Compiling the client audiostreamc file"
	gcc -c audiostreamc.c

audiostreams.o: audiostreams.c ../lib/socket_utils.h ../lib/socket_utils.h ../lib/constants.h
	@echo "Compiling the client audiostreams file"
	gcc -c audiostreams.c

socket_utils.o: ../lib/socket_utils.c ../lib/socket_utils.h
	@echo "Compiling the socket util files"
	gcc -c ../lib/socket_utils.c

congestion_control.o: ../lib/congestion_control.c ../lib/socket_utils.h
	@echo "Compiling the congestion_control files"
	gcc -c ../lib/congestion_control.c

clean:
	@echo "Removing everything but the source files"
	rm audiostreamc.o audiostreams.o socket_utils.o congestion_control.o audiostreamc.bin audiostreams.bin