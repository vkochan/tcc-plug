TARGET = libtcc-plug.so

SRCS := main.c

OBJS := $(SRCS:.c=.o)

LDFLAGS = -ltcc -lrt -ldl -shared
CC ?= tcc

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) -c -o $@ $<

.PHONY: tests
tests: $(TARGET)
	$(MAKE) -C tests/
