all: fatso

CFLAGS := $(CFLAGS) -std=gnu99 -O0 -g -Wall -pedantic -Werror

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	CFLAGS += -Wno-gnu-zero-variadic-macro-arguments
	ARCHIVE_COMMAND = libtool -static -o
endif
ifeq ($(UNAME_S),Linux)
	CFLAGS += -D_POSIX_C_SOURCE=200809L -D_GNU_SOURCE
	ARCHIVE_COMMAND = ar rcs
endif

HEADERS := fatso.h util.h internal.h
SOURCES := \
	build.c \
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
	 $(ARCHIVE_COMMAND) $@ $^

fatso: main.o libfatso.a
	$(CC) $(LDFLAGS) -o $@ $^ -lyaml

test/test: test/test.o libfatso.a test/test.h
	$(CC) $(LDFLAGS) -o $@ $< libfatso.a -lyaml

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
