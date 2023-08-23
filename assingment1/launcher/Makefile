#Compile the launch.c by linking it with the lib_simpleloader.so
all : launch


launch : launch.c
	gcc -m32 -o launch launch.c -L../loader -lloader -Wl,-rpath,'$$ORIGIN/../loader'

#Provide the command for cleanup

clean : 
	rm  launch 
