# vim: noet:

.SUFFIXES: .c
.PHONY: deps

C 			= gcc
CFLAGS 		= -Wall -fpic -g -c -I./deps/lua53/include
LUA_LIB 	= -L./deps/lua53/lib -llua -Wl,-R./deps/lua53/lib
LIB 		= $(LUA_LIB) -lm -ldl
LMCFLAGS 	= -fPIC -shared -Wl,-z,defs $(LUA_LIB)
MAIN_PATH 	= "src"
BUILD_PATH 	= "bin"
VALGRIND 	= valgrind

ALL_SRC 	= $(wildcard src/*.c)
LM_SRC 		= $(wildcard src/lm_*.c)
MAIN_SRC 	= $(filter-out $(LM_SRC), $(ALL_SRC))

.c.o:
	@echo -e "\033[0;33m*\033[0m $@"
	@$(C) $(CFLAGS) $< -o $@

all:obj pre
	@$(C) $(MAIN_PATH)/*.o $(LIB) -o $(BUILD_PATH)/main
	@echo -e "\033[0;32m[OK]\033[0m main ok"

obj:$(ALL_SRC:%.c=%.o)

lm:lm_counter lm_tsocket
	@echo -e "\033[0;32m[OK]\033[0m lua modules ok"

lm_counter:obj
	@$(C) $(LMCFLAGS) $(MAIN_PATH)/$@.o $(MAIN_PATH)/mem_pool.o -o $(BUILD_PATH)/$@.so

lm_tsocket:obj
	@$(C) $(LMCFLAGS) $(MAIN_PATH)/$@.o -o $(BUILD_PATH)/$@.so

pre:
	@mkdir -p $(BUILD_PATH)

clean:
	@rm $(MAIN_PATH)/*.o
	@rm $(BUILD_PATH)/*

run: all lm
	@$(BUILD_PATH)/main "conf/example.lua"

mem_check: all
	$(VALGRIND) \
		--tool=memcheck \
		--leak-check=yes \
		--show-reachable=yes \
		--num-callers=20 \
		--track-fds=yes \
		$(BUILD_PATH)/main

deps:
	@bash install_deps.sh
