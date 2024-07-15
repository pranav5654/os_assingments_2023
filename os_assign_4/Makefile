loader  : loader_new.c 
	gcc -m32 -no-pie -nostdlib  -o sum sum.c
	gcc -m32 -no-pie -nostdlib  -o fib fib.c
	gcc  -m32 loader_new.c -o  loader_new
	./loader_new  sum
	rm loader_new
	rm fib
	rm sum
clean:
	-@rm -f fib loader
