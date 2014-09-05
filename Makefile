all: fatso

CFLAGS := $(CFLAGS) -O0 -g -Wall -pedantic -Werror -Wno-gnu-zero-variadic-macro-arguments

HEADERS := fatso.h util.h internal.h
SOURCES := fatso.c help.c util.c project.c install.c yaml.c version.c dependency.c define.c environment.c info.c memory.c package.c repository.c
OBJECTS := $(patsubst %.c,%.o,$(SOURCES))

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

analyze: $(SOURCES) $(HEADERS)
	$(CC) --analyze $(CFLAGS) $(SOURCES)

.PHONY: clean all test analyze
