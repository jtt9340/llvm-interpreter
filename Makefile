# Makefile adapted from https://stackoverflow.com/a/20830354

#
# Compiler flags
#
CC =	clang
CXX =	clang++
CFLAGS = -Wall -Wextra -pedantic
CXXFLAGS = $(CFLAGS)
# Might need to set
# CPPFLAGS =	-I/usr/local/opt/llvm/include
# Use the libc++ bundled with Homebrew
LDFLAGS =	-L/usr/local/opt/llvm/lib -Wl,-rpath,/usr/local/opt/llvm/lib

#
# Project files
#
SRCS =	src/lexer.cpp src/main.cpp
OBJS =	$(SRCS:src/%.cpp=%.o)
EXE =	kaleidescope

#
# Debug build settings
#
DBGDIR =	target/debug
DBGEXE =	$(DBGDIR)/$(EXE)
DBGOBJS =	$(addprefix $(DBGDIR)/, $(OBJS))
DBGCFLAGS =	-g -O1 -fsanitize=address -fno-omit-frame-pointer
# Add this flag to disable tail call elimiation (helps in getting perfect stack traces)
# DBGFLAGS += -fno-optimize-sibling-calls

#
# Release build settings
#
RELDIR =	target/release
RELEXE =	$(RELDIR)/$(EXE)
RELOBJS =	$(addprefix $(RELDIR)/, $(OBJS))
RELCFLAGS =	-O2 -DNDEBUG

.PHONY:	all clean realclean debug release prep remake

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
# Other rules
#
prep:
	mkdir -p $(DBGDIR) $(RELDIR)

remake:	realclean all

clean:
	rm -f $(RELOBJS) $(DBGOBJS)

realclean:	clean
	rm -f $(RELEXE) $(DBGEXE)
