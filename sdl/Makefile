CFLAGS := -O2 -flto -Wall -Wextra $(CFLAGS_EXT)
LDFLAGS := -flto $(LDFLAGS_EXT)
CC ?= cc

ROOT ?= $(shell pwd)

SRCS := $(wildcard $(ROOT)/*.c)

OBJS := $(patsubst $(ROOT)/%.c, $(ROOT)/build/%.o, $(SRCS))

.PHONY: sdlgb

sdlgb: $(ROOT)/build/sdlgb

$(ROOT)/build:
	mkdir -v $(ROOT)/build

$(ROOT)/build/sdlgb: $(OBJS)
	cc $(LDFLAGS) -o $@ -v $^

$(ROOT)/build/%.o: $(ROOT)/%.c $(ROOT)/build
	$(CC) $(CFLAGS) -o $@ -c $<

$(ROOT)/build/%.d: $(ROOT)/%.c $(ROOT)/build
	$(CC) $(CFLAGS) -MM $< $(patsubst $(ROOT)/%.c, $(ROOT)/build/%.o, $<) -MF $@

.PHONY: clean

clean:
	rm -rvf $(ROOT)/build

ifneq ($(MAKECOMDGOALS),clean)
-include $(patsubst $(ROOT)/%.c, $(ROOT)/build/%.d, $(SRCS))
endif
