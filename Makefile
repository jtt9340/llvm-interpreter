CC =	clang
CXX =	clang++
RM =	rm -f -v

CFLAGS = -Wall -Wextra -pedantic
CXXFLAGS = $(CFLAGS)

all:	lexer.o kaleidescope

lexer.o:	lexer.cpp lexer.h
	$(CXX) $(CFLAGS) -c lexer.cpp

kaleidescope:	lexer.o
	$(CXX) $(CFLAGS) -o kaleidescope main.cpp lexer.o

clean:
	$(RM) lexer.o

realclean:	clean
	$(RM) kaleidescope
