CC = gcc
CFLAGS = -L. -lm -std=c99
OBJS = myfs.o
EXEC = myfs_manager
LIB = libmyfs.a

.PHONY: lib manager clean

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	@rm $(OBJS) $(EXEC) $(LIB)

$(LIB):$(OBJS)
	@ar rcs $(LIB) $(OBJS)

lib:$(LIB)

myfs_manager: $(LIB)
	gcc -o $@ $@.c -lmyfs $(CFLAGS)

manager:myfs_manager
