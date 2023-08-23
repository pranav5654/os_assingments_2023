all :
	$(MAKE) -C loader
	$(MAKE) -C launcher
	$(MAKE) -C test

	@echo "#################################################"
	./launcher/launch test/fib
	@echo "#################################################"
	cp launcher/launch bin
	cp loader/libloader.so bin

	
clean :
	$(MAKE) -C test clean
	$(MAKE) -C loader clean
	$(MAKE) -C launcher clean
	rm bin/*
	

#invoke make inside following directories and in this order: loader, launch, fib
#move the lib_simpleloader.so and launch binaries inside bin directory
#Provide the command for cleanup
