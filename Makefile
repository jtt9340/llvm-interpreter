# Makefile adapted from https://stackoverflow.com/a/20830354

#
# Other varibles about the environment this is being executed in
#
UNAME := $(shell uname)

ifeq ($(UNAME),Darwin)
remove-executables =                           \
	if [ -d $(1) ]; then                       \
		find $(1) -type f -perm +111 -delete;  \
	fi
else
remove-executables =                           \
	if [ -d $(1) ]; then                       \
		find $(1) -executable -type f -delete; \
	fi
endif

remove-if-exists =       \
	if [ -d $(1) ]; then \
		rmdir $(1);      \
	fi

#
# Compiler flags
#
CC =	clang
CXX =	clang++
override CFLAGS += -Wall -Wextra -pedantic -pipe "-I$(CURDIR)/interpreter/include"
override CXXFLAGS += $(CFLAGS) $(shell llvm-config --cxxflags --ldflags --system-libs --libs core orcjit native)

ifneq ($(shell command -v brew >/dev/null 2>&1 && brew ls --versions llvm@11),)
	# Use the libc++ bundled with Homebrew
	CPPFLAGS =	-I/usr/local/opt/llvm@11/include
	LDFLAGS =	-L/usr/local/opt/llvm@11/lib -Wl,-rpath,/usr/local/opt/llvm@11/lib
endif

#
# Project files
#
SRCSROOT =	interpreter/src
SRCS =	$(wildcard $(SRCSROOT)/*.cpp)
OBJS =	$(SRCS:$(SRCSROOT)/%.cpp=%.o)
TARGETDIR =	target
EXE =	kaleidoscope

#
# Debug build settings
#
DBGDIR =	$(TARGET)/debug
DBGEXE =	$(DBGDIR)/$(EXE)
DBGOBJS =	$(addprefix $(DBGDIR)/, $(OBJS))
override DBGCFLAGS +=	-v -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer
# Add this flag to disable tail call elimiation (helps in getting perfect stack traces)
# DBGFLAGS += -fno-optimize-sibling-calls

#
# Release build settings
#
RELDIR =	$(TARGET)/release
RELEXE =	$(RELDIR)/$(EXE)
RELOBJS =	$(addprefix $(RELDIR)/, $(OBJS))
override RELCFLAGS +=	-O2 -DNDEBUG

#
# Example build settings
#
EXAMPLESRCS =	$(wildcard $(EXAMPLEDIR)/*.cpp)
EXAMPLEDIR =	examples
EXAMPLEOBJS =	$(filter-out $(DBGDIR)/main.o,$(DBGOBJS))

#
# Test build settings
#
TEST =	test

.PHONY:	clean realclean debug release remake test examples fmt

#
# Release rules
#
release:	$(RELDIR) $(RELEXE)

$(RELEXE):	$(RELOBJS)
	$(CXX) $(CXXFLAGS) $(RELCFLAGS) -o $(RELEXE) $^

$(RELDIR)/%.o:	$(SRCSROOT)/%.cpp
	$(CXX) -c $(CXXFLAGS) $(RELCFLAGS) -o $@ $<

#
# Debug rules
#
debug:	$(DBGDIR) $(DBGEXE)

$(DBGEXE):	$(DBGOBJS)
	$(CXX) $(CXXFLAGS) $(DBGCFLAGS) -o $(DBGEXE) $^

$(DBGDIR)/%.o:	$(SRCSROOT)/%.cpp
	$(CXX) -c $(CXXFLAGS) $(DBGCFLAGS) -o $@ $<

#
# Example rules
#
examples:	$(patsubst $(EXAMPLEDIR)/%.cpp,$(DBGDIR)/%,$(EXAMPLESRCS))

$(DBGDIR)/%:	$(EXAMPLEDIR)/%.cpp $(EXAMPLEOBJS)
	$(CXX) $(CXXFLAGS) $(DBGCFLAGS) -o $@ $^

#
# Other rules
#

# TODO JOEY There is probably a better way to define
# the next three rules
$(TARGET):
	@mkdir -v $(TARGET)

$(DBGDIR):	$(TARGET)
	@mkdir -v $(DBGDIR)

$(RELDIR):	$(TARGET)
	@mkdir -v $(RELDIR)

test:	debug examples
	bash $(TEST)/lexer.sh
	$(DBGEXE) < $(TEST)/kaleidoscope_input.txt

clean:
	@rm -v -f $(RELDIR)/*.o $(DBGDIR)/*.o

realclean:	clean
	$(call remove-executables,$(DBGDIR))
	$(call remove-executables,$(RELDIR))
	$(call remove-if-exists,$(DBGDIR))
	$(call remove-if-exists,$(RELDIR))
	$(call remove-if-exists,$(TARGET))

remake:	realclean release

fmt:
	command -v nixfmt >/dev/null 2>&1 && nixfmt --width=80 shell.nix
	clang-format --style=llvm -i $(SRCS) \
		$(filter-out src/main.h src/KaleidoscopeJIT.h,$(SRCS:src/%.cpp=src/%.h)) \
		$(EXAMPLESRCS)
	shfmt -s -w -i 2 -ci test/lexer.sh
