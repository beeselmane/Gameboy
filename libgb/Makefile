CFLAGS := -O2 -flto -Wall -Wextra -I. -Wno-unused-command-line-argument -Wno-unused-parameter
LIBTOOL ?= libtool
CC ?= cc

ROOT ?= $(shell pwd)

SRCS := $(wildcard $(ROOT)/src/*.c)

OBJS := $(patsubst $(ROOT)/src/%.c, $(ROOT)/build/%.o, $(SRCS))

.PHONY: libgb

libgb: $(ROOT)/build/libgb.a

$(ROOT)/build:
	mkdir -v $(ROOT)/build

$(ROOT)/build/libgb.a: $(OBJS)
	libtool -static -o $@ -v $^

$(ROOT)/build/%.o: $(ROOT)/src/%.c $(ROOT)/build
	$(CC) $(CFLAGS) -o $@ -c $<

$(ROOT)/build/%.d: $(ROOT)/src/%.c $(ROOT)/build
	$(CC) $(CFLAGS) -MM $< $(patsubst $(ROOT)/src/%.c, $(ROOT)/build/%.o, $<) -MF $@

.PHONY: clean

clean:
	rm -rvf $(ROOT)/build

ifneq ($(MAKECOMDGOALS),clean)
-include $(patsubst $(ROOT)/src/%.c, $(ROOT)/build/%.d, $(SRCS))
endif
