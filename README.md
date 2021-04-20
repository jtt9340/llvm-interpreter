# LLVM-Powered Scripting Language Interpreter
This is a work-in-progress scripting language interpreter that uses LLVM to emit
LLVM IR, which is then JIT-compiled into native code.

The language, called _Kaleidoscope_ after the [LLVM Tutorial], is an _expression oriented_, _imperative_ scripting language.
This means everything in Kaleidoscope evaluates to an expression, and every expression evaluates to a
value. Every expression is terminated with a semicolon `;`. Currently, the only values are double-precision floating point
numbers. Boolean values are simulated by having 0.0 evaluate to a false-y (falseful?) value, and every other number is truth-y/truthful.

## Now playing at Kaleidoscope cinema:
### Functions
Functions begin with the `def` keyword followed by a function name. Valid function names start with an upper
or lowercase letter `[A-Za-z]`, an underscore `_`, or a dollar sign `$`, followed by more upper or lowercase letters,
underscores, and dollar signs, as well as numbers `[0-9]`. What follows must be a parameter list, i.e. zero or more identifiers
encased in parenthesis and separated by whitespace. This is different from most languages that choose to have their function
declaration and invocation syntaxes mimic mathematical function notation--where a comma separates identifers--but in this case,
all you need is whitespace. The rules for valid function paramter names are the same as rules for valid function names.

As mentioned earlier, Kaleidoscope is an expression oriented language: the body of a function is a single expression, so there is
no `return` statement or even a `return` keyword, as the function takes on the value of the expression it contains. Since expressions
must end with semicolons, a function declaration is terminated by one too.

```
def quadratic(a b c x)
    a*x*x + b*x + c;
```

Calling functions _do_ requires commas, however, to separate expressions.

```
quadratic(4, 5-7, .0, 1);
```

### Extern(al) Functions
Functions can also be declared with the `extern` keyword instead of the `def` keyword.
In this case, they do not contain a body. Syntactically they are very similar to function
headers in C.
```
extern sin(x);
extern putchard(c);
```
Extern functions are called like normal ones. Kaleidoscope looks for symbols it has been
linked against when invoking an extern function. In this case, `sin` comes from C's
math library and `putchard` is a Kaleidoscope "standard library" function that
prints out a character given its ASCII code. You can define your own functions natively
to be used by Kaleidoscope. For example, here is how `putchard` was defined:
```C++
#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

extern "C" DLLEXPORT double putchard(double c) {
    std::fputc(static_cast<char>(c), stderr);
    return 0.0; // Every expression must evaluate to a value
}
```
Remember that currently the only type of value in Kaleidoscope is the `double` precision floating point number,
so any external function defined must take zero or more `double`s and return a `double`, in order for the function
call to evaluate to a value.

### Comments
Comments start with a pound-sign/hashtag/octothorpe/tic-tac-toe board `#` and go until the end of the line, like in Python or Bash.
Comments may appear on their own line, or at the end of a line with code on it.

```
# Copyright (c) 2021 Joey Territo

quadratic(0-1, 0.8, 6.8-11, 2); # Evaluates to -6.6 assuming the function definition above
```

### Arithmetic
```
1.23 + 4.56;

21-42;

100 * 0.70;

3 / 4;
```

### Comparison Operators
Unfortunately, only `<` and `>` are supported right now, but more will be implemented in the future.
Comparison operators evaluate to either 0.0 or 1.0 to represent false and true, respectively.

```
def is_positive(x) x > 0.0;

def is_negative(x) x < 0.0;

is_positive(8.8); # Evaluates to 1.0

is_negative(8.8); # Evaluates to 0.0
```

### If Expressions
Notice these are if _expressions_, not _statements_--if expressions in Kaleidoscope behave like
the ternary operator in C, which means the `else` clause is necessary.

```
def abs(v)
    if is_positive(v) then
        v
    else
        v * (0-1)
    ; # Don't forget the semicolon!
```

### For Loops
Kaleidoscope's for loops are the familiar conditional loops that evaluate an initial expression,
execute a body while another expression is true, running another line of code at the end of each iteration.
They look like this:

```
for i = 0, i < 10 in
    putchard(42);
```

This does what you expect: it sets `i` to 0 once before the loop is run, then calls `putchard(42)` while
`i < 10`. The `in` keyword separates the for loop delcaration from the body of code to run in the loop.

You might have noticed that `i` is never incremented, yet this loop will terminate. That is because `for`
loops have an optional "step" clause that can appear after the condition that indicates how much to increment
your variable by at the end of each iteration. When that step is not included, it is assumed to be 1.0.

```
# Skip count by 2
for i = 0, i < 10, 2 in
    putchard(42 + i);
```

You can find the syntax of Kaleidoscope altogether in the sample file [test/kaleidoscope_input.txt](test/kaleidoscope_input.txt).

## Building From Source
Ensure LLVM is installed on your machine. On a Mac with [Homebrew]: `brew install llvm`.

The project is controlled by a single Makefile. In the root of this repository, run

```
make
```

The resulting binary is called `kaleidoscope` and lives in the `target/release` directory.
You can also run

```
make debug
```

to get a debug build in the `target/debug` directory, which builds with debug symbols suitable for [LLDB] as well as [Address Sanitizer][AddressSanitizer]
(for getting an informative stacktraces for segfaults) and [Undefined Behavior Sanitizer][UBSan] (for reporting usage of
undefined behavior).

Additionally, there is an [examples](examples) directory that contains self-contained programs that test certain components of the
interpreter. You can build those with `make examples`. The resulting binaries will be found in `target/debug`.

Finally, there is a rudimentary test that tests the lexer and that the interpreter doesn't crash with the aforementioned
[test/kaleidoscope_input.txt](test/kaleidoscope_input.txt): `make test`.

[LLVM Tutorial]: https://llvm.org/docs/tutorial/index.html
[Homebrew]: https://brew.sh
[LLDB]: https://lldb.llvm.org
[AddressSanitizer]: https://github.com/google/sanitizers/wiki/AddressSanitizer
[UBSan]: https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html
