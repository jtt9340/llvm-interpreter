#include <cstdio>  // std::fprintf

#include "lexer.h" // SetupBinopPrecedences, getNextToken, MainLoop

int main(int argc, const char **argv) {
	const char *argv0 = argv[0];
	SetupBinopPrecedences();
	InitializeModuleAndPassManager();

	// Get ready to parse the first token.
	std::fprintf(stderr, "%s> ", argv0);
	getNextToken();

	// Run the REPL now.
	MainLoop(argv0);

	return 0;
}
