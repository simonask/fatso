all: fatso

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

libfatso.a: fatso.o parser.o help.o
	libtool -static -o $@ $^

fatso: main.o libfatso.a
	$(CC) -o $@ -lyaml $^

clean:
	rm -f fatso
	rm -f libfatso.a
	rm -f *.o

.PHONY := clean all
