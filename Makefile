#Create lib_simpleloader.so from loader.c

all: libloader.so

libloader.so : loader.o
	gcc  -Wall -Werror  -m32 -shared -o libloader.so  loader.o
	rm *.o

loader.o: loader.c
	gcc -m32 -c loader.c -o loader.o

#Provide the command for cleanup
clean :
	rm  libloader.so

