# Makefile adapted from https://stackoverflow.com/a/20830354

#
# Other varibles about the environment this is being executed in
#
UNAME := $(shell uname)

#
# Compiler flags
#
CC =	clang
CXX =	clang++
CFLAGS = -Wall -Wextra -pedantic -pipe 
CXXFLAGS = $(CFLAGS) $(shell llvm-config --cxxflags --ldflags --system-libs --libs core orcjit native)

ifneq ($(shell command -v brew >/dev/null 2>&1 && brew ls --versions llvm@11),)
	# Use the libc++ bundled with Homebrew
	CPPFLAGS =	-I/usr/local/opt/llvm@11/include
	LDFLAGS =	-L/usr/local/opt/llvm@11/lib -Wl,-rpath,/usr/local/opt/llvm@11/lib
endif

#
# Project files
#

# Update this variable as more files are added to the project
SRCS =	src/ast.cpp src/KaleidoscopeJIT.cpp src/lexer.cpp src/logging.cpp src/main.cpp src/parser.cpp
OBJS =	$(SRCS:src/%.cpp=%.o)
EXE =	kaleidoscope

#
# Debug build settings
#
DBGDIR =	target/debug
DBGEXE =	$(DBGDIR)/$(EXE)
DBGOBJS =	$(addprefix $(DBGDIR)/, $(OBJS))
DBGCFLAGS =	-v -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer
# Add this flag to disable tail call elimiation (helps in getting perfect stack traces)
# DBGFLAGS += -fno-optimize-sibling-calls

#
# Release build settings
#
RELDIR =	target/release
RELEXE =	$(RELDIR)/$(EXE)
RELOBJS =	$(addprefix $(RELDIR)/, $(OBJS))
RELCFLAGS =	-O2 -DNDEBUG

#
# Example build settings
#
EXAMPLEDIR =	examples
EXAMPLEOBJS =	$(filter-out $(DBGDIR)/main.o,$(DBGOBJS))

.PHONY:	all clean realclean remove-executables debug release prep remake test examples fmt fmt-helper

# Default build
all:	prep release

#
# Debug rules
#
debug:	$(DBGEXE)

$(DBGEXE):	$(DBGOBJS)
	$(CXX) $(CXXFLAGS) $(DBGCFLAGS) -o $(DBGEXE) $^

$(DBGDIR)/%.o:	src/%.cpp
	$(CXX) -c $(CXXFLAGS) $(DBGCFLAGS) -o $@ $<

#
# Release rules
#
release:	$(RELEXE)

$(RELEXE):	$(RELOBJS)
	$(CXX) $(CXXFLAGS) $(RELCFLAGS) -o $(RELEXE) $^

$(RELDIR)/%.o:	src/%.cpp
	$(CXX) -c $(CXXFLAGS) $(RELCFLAGS) -o $@ $<

#
# Example rules
#
examples:	$(patsubst $(EXAMPLEDIR)/%.cpp,$(DBGDIR)/%,$(wildcard $(EXAMPLEDIR)/*.cpp))

$(DBGDIR)/%:	$(EXAMPLEDIR)/%.cpp $(EXAMPLEOBJS)
	$(CXX) $(CXXFLAGS) $(DBGCFLAGS) -o $@ $^

#
# Other rules
#
prep:
	mkdir -p $(DBGDIR) $(RELDIR)

test:	debug examples
	bash test/lexer.sh
	$(DBGEXE) < test/kaleidoscope_input.txt

remake:	realclean all

clean:
	@rm -v -f $(RELOBJS) $(DBGOBJS)

ifeq ($(UNAME),Darwin)
remove-executables:
	find $(DBGDIR) -type f -perm +111 -delete
	find $(RELDIR) -type f -perm +111 -delete
else
remove-executables:
	find $(DBGDIR) -executable -type f -delete 
	find $(RELDIR) -executable -type f -delete 
endif

realclean:	clean remove-executables
	@rm -v -f $(RELEXE) $(DBGEXE)

fmt-helper:
	clang-format --style=llvm -i $(SRCS) \
		$(filter-out src/main.h src/KaleidoscopeJIT.h,$(SRCS:src/%.cpp=src/%.h)) \
		$(wildcard $(EXAMPLEDIR)/*.cpp)
	shfmt -s -w -i 2 -ci test/lexer.sh

ifneq (shell command nixfmt --help,)
fmt: fmt-helper
	nixfmt --width=80 shell.nix
else
fmt: fmt-helper
endif
