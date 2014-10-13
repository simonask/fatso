all: test-project

%.o: %.c
	$(CC) $(CFLAGS) -Werror -c -o $@ $<

test-project: project.o
	$(CC) $(LDFLAGS) -o $@ $^ -lyaml

clean:
	rm -f test-project *.o

.PHONY: all clean
