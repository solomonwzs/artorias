# vim: noet:

.SUFFIXES: .c

C 			= gcc
CFLAGS 		= -Wall -fpic -g -c
LIB 		= -llua
MAIN_PATH 	= "src"
MAIN_SRC 	= $(wildcard src/*.c)
LM_PATH 	= "src/lua"
LM_SRC 		= $(wildcard src/lua/*.c)
BUILD_PATH 	= "bin"
VALGRIND 	= valgrind

.c.o:
	@echo -e $@
	@$(C) $(CFLAGS) $< -o $@

%.so:
	@echo -e $@
	@$(C) $(CFLAGS) $(LIB) -shared -o $@ $(@:%.so=%.c)
	@mv $@ $(BUILD_PATH)

all:$(MAIN_SRC:%.c=%.o) pre
	@$(C) $(MAIN_PATH)/*.o $(LIB) -o $(BUILD_PATH)/main
	@echo -e "all ok"

lm:$(LM_SRC:%.c=%.so)
	@echo -e "all ok"

pre:
	@mkdir -p $(BUILD_PATH)

clean:
	@rm $(MAIN_PATH)/*.o
	@rm $(LM_PATH)/*.o
	@rm $(BUILD_PATH)/*

run: all
	@$(BUILD_PATH)/main "conf/example.lua"

mem_check: all
	$(VALGRIND) \
		--tool=memcheck \
		--leak-check=yes \
		--show-reachable=yes \
		--num-callers=20 \
		--track-fds=yes \
		$(BUILD_PATH)/main

t:
	@echo "$(wildcard src/*.c)"
