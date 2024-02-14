all: ssftp ssftpd

ssftp: ssftp.o message_utils.o socket_utils.o constants.h
	@echo "Linking files and creating the client executable"
	gcc ssftp.o message_utils.o socket_utils.o -o ssftp.bin

ssftpd: ssftpd.o message_utils.o socket_utils.o constants.h
	@echo "Linking files and creating the server executable"
	gcc ssftpd.o message_utils.o socket_utils.o -o ssftpd.bin

ssftp.o: ssftp.c message_utils.h socket_utils.h constants.h
	@echo "Compiling the client file"
	gcc -c ssftp.c

ssftpd.o: ssftpd.c message_utils.h socket_utils.h constants.h
	@echo "Compiling the server file"
	gcc -c ssftpd.c

message_utils.o: message_utils.c message_utils.h
	@echo "Compiling the util file"
	gcc -c message_utils.c

socket_utils.o: socket_utils.c socket_utils.h
	@echo "Compiling the util file"
	gcc -c socket_utils.c

clean:
	@echo "Removing everything but the source files"
	rm ssftpd.o ssftp.o message_utils.o socket_utils.o ssftp.bin ssftpd.bin