all: fatso

CFLAGS := $(CFLAGS) -O0 -g -Wall -pedantic -Werror -Wno-gnu-zero-variadic-macro-arguments

HEADERS := fatso.h util.h internal.h
SOURCES := \
	define.c \
	dependency.c \
	configuration.c \
	env.c \
	exec.c \
	fatso.c \
	help.c \
	info.c \
	install.c \
	memory.c \
	package.c \
	process.c \
	project.c \
	repository.c \
	source.c \
	sync.c \
	toolchain.c \
	util.c \
	version.c \
	yaml.c \

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
	$(CC) --analyze $(CFLAGS) -Xanalyzer -analyzer-output=text $(SOURCES)

.PHONY: clean all test analyze
