CC=gcc
LD=gcc

WARN=-Wall -Wextra -Wno-pedantic -Wshadow -Wstack-protector -Wformat=2 -Wconversion -Wformat-security -Wunused-parameter -Werror -Wno-sign-conversion
CFLAGS=$(WARN) -O0 -g -march=native -fsanitize=address,undefined -Isrc/include

LDFLAGS=-lasan -lubsan

src=
src+=$(shell find -O3 src -name '*.c')
src+=$(shell find -O3 src -name '*.h')

obj=$(patsubst src/%.c,build/%.o, $(src))

all: bin/twSyncServer

bin/twSyncServer: $(obj)
	@mkdir -p $(@D)
	@$(info LD    $(subst build/,,$^) ==> $@)
	@$(LD) $(LDFLAGS) -o $@ $^

build/%.o: src/%.c
	@mkdir -p $(@D)
	@$(info CC    $(subst src/,,$^) ==> $@)
	@$(CC) $(CFLAGS) -c $^ -o $@
clean:
	@rm -rf build
	@rm -rf bin