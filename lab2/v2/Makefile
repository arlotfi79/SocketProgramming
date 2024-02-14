all: ssftp ssftpd

ssftp: ssftp.o socket_utils.o constants.h
	@echo "Linking files and creating the client executable"
	gcc ssftp.o socket_utils.o -o ssftp.bin

ssftpd: ssftpd.o socket_utils.o constants.h
	@echo "Linking files and creating the server executable"
	gcc ssftpd.o socket_utils.o -o ssftpd.bin

ssftp.o: ssftp.c socket_utils.h constants.h
	@echo "Compiling the client file"
	gcc -c ssftp.c

ssftpd.o: ssftpd.c socket_utils.h constants.h
	@echo "Compiling the server file"
	gcc -c ssftpd.c

socket_utils.o: socket_utils.c socket_utils.h
	@echo "Compiling the util file"
	gcc -c socket_utils.c

clean:
	@echo "Removing everything but the source files"
	rm ssftpd.o ssftp.o socket_utils.o ssftp.bin ssftpd.bin