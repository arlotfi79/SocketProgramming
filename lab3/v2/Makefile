all: ssftp ssftpd

ssftp: ssftp.o socket_utils.o ack_handling.o ack_handling.h constants.h
	@echo "Linking files and creating the client executable"
	gcc ssftp.o socket_utils.o ack_handling.o -o ssftp.bin

ssftpd: ssftpd.o socket_utils.o ack_handling.o ack_handling.h constants.h
	@echo "Linking files and creating the server executable"
	gcc ssftpd.o socket_utils.o ack_handling.o -o ssftpd.bin

ssftp.o: ssftp.c socket_utils.h ack_handling.h constants.h
	@echo "Compiling the client file"
	gcc -c ssftp.c

ssftpd.o: ssftpd.c socket_utils.h ack_handling.h constants.h
	@echo "Compiling the server file"
	gcc -c ssftpd.c

socket_utils.o: socket_utils.c socket_utils.h
	@echo "Compiling the util file"
	gcc -c socket_utils.c

ack_handling.o: ack_handling.c ack_handling.h
	@echo "Compiling the util file"
	gcc -c ack_handling.c

clean:
	@echo "Removing everything but the source files"
	rm ssftpd.o ssftp.o socket_utils.o ack_handling.o ssftp.bin ssftpd.bin