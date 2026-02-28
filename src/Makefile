CC=gcc
CFLAGS=
FMT=indent

mysh: shell.c interpreter.c shellmemory.c scripts.c scheduler.c
	$(CC) $(CFLAGS) -c shell.c interpreter.c shellmemory.c scripts.c scheduler.c
	$(CC) $(CFLAGS) -o mysh shell.o interpreter.o shellmemory.o scripts.o scheduler.o -lpthread

style: shell.c shell.h interpreter.c interpreter.h shellmemory.c shellmemory.h
	$(FMT) $?

clean: 
	$(RM) mysh; $(RM) *.o; $(RM) *~

