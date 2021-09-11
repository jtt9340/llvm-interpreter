# Makefile adapted from https://stackoverflow.com/a/20830354

#
# Other varibles about the environment this is being executed in
#
UNAME := $(shell uname)

#
# Helper functions
#
# Retrieve all executable files from the given directory
ifeq ($(UNAME),Darwin)
find-executables =                    \
	if [ -d $(1) ]; then              \
		find $(1) -type f -perm +111; \
	fi
else
find-executables =                     \
	if [ -d $(1) ]; then               \
		find $(1) -executable -type f; \
	fi
endif

# Remove all executable files (except *.sh files) from the given directory
remove-executables =                                 \
	for exe in $$($(call find-executables,$(1))); do \
		if ! echo $$exe | grep -q '.sh$$'; then      \
			rm -f -v "$$exe";                        \
		fi;                                          \
	done

# Remove the given directory if it exists
remove-if-exists =       \
	if [ -d $(1) ]; then \
		rmdir $(1);      \
	fi

#
# Project files
#
SRCSROOT =	interpreter/src
INCLUDEROOT = 	interpreter/include
SRCS =	$(wildcard $(SRCSROOT)/*.cpp)
OBJS =	$(SRCS:$(SRCSROOT)/%.cpp=%.o)
TARGETDIR =	target
EXE =	kaleidoscope

#
# Compiler flags
#
CC =	clang
CXX =	clang++
override CFLAGS += -Wall -Wextra -pedantic -pipe "-I$(CURDIR)/$(INCLUDEROOT)"
override CXXFLAGS += $(CFLAGS) $(shell llvm-config --cxxflags --ldflags --system-libs --libs all)

ifneq ($(shell command -v brew >/dev/null 2>&1 && brew ls --versions llvm@11),)
	# Use the libc++ bundled with Homebrew
	CPPFLAGS =	-I/usr/local/opt/llvm@11/include
	LDFLAGS =	-L/usr/local/opt/llvm@11/lib -Wl,-rpath,/usr/local/opt/llvm@11/lib
endif

#
# Debug build settings
#
DBGDIR =	$(TARGETDIR)/debug
DBGEXE =	$(DBGDIR)/$(EXE)
DBGOBJS =	$(addprefix $(DBGDIR)/, $(OBJS))
override DBGCFLAGS +=	-v -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer
# Add this flag to disable tail call elimiation (helps in getting perfect stack traces)
ifdef NOTAILCALL
override DBGCFLAGS += -fno-optimize-sibling-calls
endif

#
# Release build settings
#
RELDIR =	$(TARGETDIR)/release
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
TESTDIR =	test
TESTSRCS =	$(wildcard $(TESTDIR)/*.cpp)
TESTOBJS =	$(EXAMPLEOBJS)

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
# Test rules
#
test:	debug $(DBGDIR)/lexer $(patsubst $(TESTDIR)/%.cpp,$(TESTDIR)/%,$(TESTSRCS))
	@for unittest in $$($(call find-executables,$(TESTDIR))); do \
		echo "$$unittest" && "$$unittest";                       \
	done
	ASAN_OPTIONS=detect_container_overflow=0 $(DBGEXE) < $(TESTDIR)/kaleidoscope_input.txt

$(TESTDIR)/%:	$(TESTDIR)/%.cpp $(TESTOBJS)
	$(CXX) $(CXXFLAGS) $(DBGCFLAGS) -o $@ $^

#
# Other rules
#

# TODO JOEY There is probably a better way to define
# the next three rules
$(TARGETDIR):
	@mkdir -v $(TARGETDIR)

$(DBGDIR):	$(TARGETDIR)
	@mkdir -v $(DBGDIR)

$(RELDIR):	$(TARGETDIR)
	@mkdir -v $(RELDIR)

clean:
	@rm -v -f $(RELDIR)/*.o $(DBGDIR)/*.o

realclean:	clean
# TODO JOEY Can this be a loop?
	@$(call remove-executables,$(DBGDIR))
	@$(call remove-executables,$(RELDIR))
	@$(call remove-executables,$(TESTDIR))
ifeq ($(UNAME),Darwin)
	@find "$(CURDIR)" \( -type f -name .DS_Store -o -type d -name __MACOSX \) -print0 | \
		xargs -0 rm -r -v -f
	@if [ -d $(DBGDIR) ]; then                                   \
		find $(DBGDIR) -name *.dSYM -print0 | xargs -0 rm -r -v; \
	fi	
	@if [ -d $(RELDIR) ]; then                                   \
		find $(RELDIR) -name *.dSYM -print0 | xargs -0 rm -r -v; \
	fi	
	@find $(TESTDIR) -name *.dSYM -print0 | xargs -0 rm -r -v 
endif
	@$(call remove-if-exists,$(DBGDIR))
	@$(call remove-if-exists,$(RELDIR))
	@$(call remove-if-exists,$(TARGETDIR))

remake:	realclean release

fmt:
	@if command -v nixfmt >/dev/null 2>&1; then \
		echo nixfmt --width=80 shell.nix; \
		nixfmt --width=80 shell.nix; \
	fi
	clang-format --style=llvm -i $(SRCS) \
		$(filter-out $(INCLUDEROOT)/main.h $(INCLUDEROOT)/KaleidoscopeJIT.h,$(SRCS:$(SRCSROOT)/%.cpp=$(INCLUDEROOT)/%.h)) \
		$(EXAMPLESRCS) $(TESTSRCS)
	shfmt -s -w -i 2 -ci test/*.sh
