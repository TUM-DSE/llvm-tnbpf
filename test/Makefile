%.bpf.o: %.bpf.c
	../build-debug/bin/clang -mllvm -debug -O2 -g -I"${LINUX_HEADERS}/include" -I"${LIBBPF}/include" -target bpf -c $^ -o $@

# https://stackoverflow.com/a/79685650
SOURCES := $(wildcard *.bpf.c)
OBJS    := $(patsubst %.c,%.o,$(notdir $(SOURCES)))

all: $(OBJS)

clean:
	rm $(OBJS)

