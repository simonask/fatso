all: fatso

HEADERS := fatso.h util.h

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

libfatso.a: fatso.o parser.o help.o util.o project.o install.o
	libtool -static -o $@ $^

fatso: main.o libfatso.a
	$(CC) -o $@ -lyaml $^

clean:
	rm -f fatso
	rm -f libfatso.a
	rm -f *.o

.PHONY := clean all
