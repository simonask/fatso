all: fatso

CFLAGS := $(CFLAGS) -std=gnu99 -fPIC -O0 -g -Wall -pedantic -Wno-unused -Werror

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	CFLAGS += -Wno-gnu-zero-variadic-macro-arguments
	TEST_LDFLAGS = $(LDFLAGS) -force_flat_namespace
	ARCHIVE_COMMAND = libtool -static -o
	SHLIBEXT = dylib
	PRELOAD_ENV = DYLD_INSERT_LIBRARIES
endif
ifeq ($(UNAME_S),Linux)
	CFLAGS += -D_POSIX_C_SOURCE=200809L -D_GNU_SOURCE
	TEST_LDFLAGS = $(LDFLAGS)
	ARCHIVE_COMMAND = ar rcs
	SHLIBEXT = so
	PRELOAD_ENV = LD_PRELOAD
endif

HEADERS := fatso.h util.h internal.h
SOURCES := \
	autotools.c \
	build.c \
	dependency.c \
	configuration.c \
	env.c \
	exec.c \
	fatso.c \
	git.c \
	help.c \
	info.c \
	install.c \
	make.c \
	memory.c \
	package.c \
	process.c \
	project.c \
	repository.c \
	scons.c \
	source.c \
	sync.c \
	tarball.c \
	toolchain.c \
	util.c \
	version.c \
	yaml.c \

OBJECTS := $(patsubst %.c,%.o,$(SOURCES))

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

libfatso.a: $(OBJECTS)
	$(ARCHIVE_COMMAND) $@ $^

libfatso.$(SHLIBEXT): $(OBJECTS)
	$(CC) $(LDFLAGS) -shared -o $@ $^ -lyaml

fatso: main.o libfatso.dylib
	$(CC) $(LDFLAGS) -o $@ $^ -lyaml -L. -lfatso

TEST_INTERCEPTOR = test/libtest-interceptor.$(SHLIBEXT)

$(TEST_INTERCEPTOR): test/interceptor.o libfatso.$(SHLIBEXT)
	$(CC) $(CFLAGS) $(LDFLAGS) -Os -shared -Werror -o $@ $< -L. -lfatso

test/test: test/test.o libfatso.$(SHLIBEXT) test/test.h $(TEST_INTERCEPTOR)
	$(CC) $(TEST_LDFLAGS) -o $@ $< -L. -lfatso -lyaml

test: test/test
	exec env $(PRELOAD_ENV)=$(TEST_INTERCEPTOR) test/test

clean:
	rm -f fatso
	rm -f libfatso.a
	rm -f libfatso.dylib
	rm -f *.o
	rm -f test/*.o test/test

analyze: $(SOURCES) $(HEADERS)
	$(CC) --analyze $(CFLAGS) -Xanalyzer -analyzer-output=text $(SOURCES)

.PHONY: clean all test analyze
