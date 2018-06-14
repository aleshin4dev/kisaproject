LINKER        = g++
LINKERFLAGS   =  -s
COMPILER      = g++
COMPILERFLAGS =  -std=c++14 -Wall
BIN           = slexscan-test
LIBS          = -lboost_filesystem -lboost_system
vpath %.cpp src
vpath %.o build
OBJ           = slexscan-test.o print_lexem.o char_trie.o slexscan.o search_char.o error_count.o file_contents.o fsize.o get_init_state.o char_conv.o
LINKOBJ       = build/slexscan-test.o build/print_lexem.o build/char_trie.o build/slexscan.o build/search_char.o build/error_count.o build/file_contents.o build/fsize.o build/get_init_state.o build/char_conv.o

.PHONY: all all-before all-after clean clean-custom

all: all-before $(BIN) all-after

clean: clean-custom 
	rm -f ./build/*.o
	rm -f ./build/$(BIN)

.cpp.o:
	$(COMPILER) -c $< -o $@ $(COMPILERFLAGS) 
	mv $@ ./build

$(BIN):$(OBJ)
	$(LINKER) -o $(BIN) $(LINKOBJ) $(LIBS) $(LINKERFLAGS)
	mv $(BIN) ./build