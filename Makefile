all: fatso

CFLAGS := $(CFLAGS) -O0 -g -Wall -pedantic -Werror -Wno-gnu-zero-variadic-macro-arguments

HEADERS := fatso.h util.h internal.h
OBJECTS := fatso.o help.o util.o project.o install.o yaml.o version.o dependency.o define.o environment.o info.o memory.o package.o

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

libfatso.a: $(OBJECTS)
	libtool -static -o $@ $^

fatso: main.o libfatso.a
	$(CC) $(LDFLAGS) -o $@ -lyaml $^

test/test: test/test.o libfatso.a test/test.h
	$(CC) $(LDFLAGS) -o $@ -lyaml $< libfatso.a

test: test/test
	exec test/test

clean:
	rm -f fatso
	rm -f libfatso.a
	rm -f *.o
	rm -f test/*.o test/test

.PHONY: clean all test
