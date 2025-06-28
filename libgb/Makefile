CFLAGS := -O2 -flto -Wall -Wextra -I.
LIBTOOL ?= libtool
CC ?= cc

SRCS := $(wildcard src/*.c)

OBJS := $(patsubst src/%.c, build/%.o, $(SRCS))

.PHONY: libgb

libgb: build/libgb.a

build:
	mkdir -v build

build/libgb.a: $(OBJS)
	libtool -static -o $@ -v $^

build/%.o: src/%.c build
	$(CC) $(CFLAGS) -o $@ -c $<

build/%.d: src/%.c build
	$(CC) $(CFLAGS) -MM $< $(patsubst src/%.c, build/%.o, $<) -MF $@

.PHONY: clean

clean:
	rm -rvf build

ifneq ($(MAKECOMDGOALS),clean)
-include $(patsubst %.c, build/%.d, $(SRCS))
endif
