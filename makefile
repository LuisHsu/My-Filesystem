CC = gcc
CFLAGS = -L. -lm -std=c99
OBJS = myfs.o
EXEC = myfs_manager
LIB = libmyfs.a

.PHONY: lib manager clean test_Superblock

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	@rm $(OBJS) $(EXEC) $(LIB)

$(LIB):$(OBJS)
	@ar rcs $(LIB) $(OBJS)

lib:$(LIB)

myfs_manager: $(LIB)
	gcc -o $@ $@.c -lmyfs $(CFLAGS)
	
Superblock_test: Superblock_test.c
	gcc -o $@ $@.c $(CFLAGS)

manager:myfs_manager
test_Superblock: Superblock_test
