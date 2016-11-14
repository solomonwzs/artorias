# vim: noet:

.SUFFIXES: .c

C 			= gcc
CFLAGS 		= -Wall -fpic -g -c
LIB 		= -llua
SOURCES		= $(wildcard ./src/*.c)
BUILD_PATH 	= "bin"
VALGRIND 	= valgrind

.c.o:
	@echo -e $@
	@$(C) $(CFLAGS) $< -o $@

all:$(SOURCES:%.c=%.o) pre
	@$(C) ./src/*.o $(LIB) -o $(BUILD_PATH)/main
	@echo -e "ok"

pre:
	@mkdir -p $(BUILD_PATH)

clean:
	@rm ./src/*.o
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
