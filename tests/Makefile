CC=clang
CFLAGS=-I .. -Weverything -Werror -Wno-padded
LDFLAGS=

all: test_datetime

test: all
	./test_datetime

test_datetime: test_datetime.o datetime.o
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $^

%.o: ../%.c
	$(CC) $(CFLAGS) -o $@ -c $^

clean:
	rm -f *.o test_datetime
