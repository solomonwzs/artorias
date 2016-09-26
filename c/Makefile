# vim: noet:

.SUFFIXES: .c

C 			= gcc
CFLAGS 		= -Wall -fpic -g -c
SOURCES		= $(wildcard ./src/*.c)
BUILD_PATH 	= "bin"

.c.o:
	@echo -e $@
	@$(C) $(CFLAGS) $< -o $@

all:$(SOURCES:%.c=%.o) pre
	@$(C) ./src/*.o -o $(BUILD_PATH)/main
	@echo -e "ok"

pre:
	@mkdir -p $(BUILD_PATH)

clean:
	@rm ./src/*.o
	@rm $(BUILD_PATH)/*
