CC = gcc
CFLAGS = -std=gnu17 -Wall -Wextra -Werror

OBJS = router.o aux.o

all: router

router: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o router

router.o: router.c router.h
	$(CC) -c $(CFLAGS) router.c -o router.o

aux.o : aux.c router.h
	$(CC) -c $(CFLAGS) aux.c -o aux.o

clean:
	rm *.o

distclean:
	rm *.o router

