CC :=gcc
LDFLAGS := -Wall
NAMESC :=$(wildcard *.c)
NAMES :=$(NAMESC:.c=)

all: $(NAMES)

%:%.c
		$(CC) $< $(LDFLAGS) $(CFLAGS) -o $@

clean:
		rm -rf $(NAMES)