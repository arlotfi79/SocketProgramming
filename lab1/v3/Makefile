all: newsh

newsh: newsh.o utils.o
	@echo "Linking files and creating the final executable"
	gcc newsh.o utils.o -o newsh.bin

newsh.o: newsh.c utils.h
	@echo "Compiling the main file"
	gcc -c newsh.c

utils.o: utils.c utils.h
	@echo "Compiling the util file"
	gcc -c utils.c

clean:
	@echo "Removing everything but the source files"
	rm newsh.o utils.o newsh.bin
