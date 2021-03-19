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

# Update this variable as more files are added to the project
SRCS =	src/ast.cpp src/lexer.cpp src/logging.cpp src/main.cpp
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

#
# Example build settings
#
EXAMPLEDIR =	examples
EXAMPLEOBJS =	$(filter-out $(DBGDIR)/main.o,$(DBGOBJS))

.PHONY:	all clean realclean debug release prep remake examples

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
# Example rules - is there a cleaner way to write this part of the makefile?
#
examples:	$(patsubst $(EXAMPLEDIR)/%.cpp,$(DBGDIR)/%,$(wildcard $(EXAMPLEDIR)/*.cpp))

$(DBGDIR)/%:	$(EXAMPLEDIR)/%.cpp $(EXAMPLEOBJS)
	$(CXX) -c $(CXXFLAGS) $(DBGCFLAGS) -o $(<:$(EXAMPLEDIR)/%.cpp=$(EXAMPLEDIR)/%.o) $<
	$(CXX) $(CXXFLAGS) $(DBGCFLAGS) -o $@ $(<:$(EXAMPLEDIR)/%.cpp=$(EXAMPLEDIR)/%.o) $(EXAMPLEOBJS)

#
# Other rules
#
prep:
	mkdir -p $(DBGDIR) $(RELDIR)

remake:	realclean all

clean:
	@rm -v -f $(RELOBJS) $(DBGOBJS)

realclean:	clean
	@rm -v -f $(RELEXE) $(DBGEXE)
