# vim: noet:

.SUFFIXES: .c

C 			= gcc
CFLAGS 		= -Wall -fpic -g -c
LIB 		= -llua
CORE_PATH 	= "src/core"
CORE_SRC	= $(wildcard src/core/*.c)
BUILD_PATH 	= "bin"
VALGRIND 	= valgrind

.c.o:
	@echo -e $@
	@$(C) $(CFLAGS) $< -o $@

all:core
	@echo -e "ok"

core:$(CORE_SRC:%.c=%.o) pre
	@$(C) $(CORE_PATH)/*.o $(LIB) -o $(BUILD_PATH)/main
	@echo -e "core ok"

pre:
	@mkdir -p $(BUILD_PATH)

clean:
	@rm $(CORE_PATH)/*.o
	@rm $(BUILD_PATH)/*

run: all
	@$(BUILD_PATH)/main

mem_check: all
	$(VALGRIND) \
		--tool=memcheck \
		--leak-check=yes \
		--show-reachable=yes \
		--num-callers=20 \
		--track-fds=yes \
		$(BUILD_PATH)/main
