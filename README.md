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
encased in parentheses and separated by whitespace. This is different from most languages that choose to have their function
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

### Used-Defined Unary and Binary Operators
Unlike many modern mainstream languages, Kaleidoscope allows the user to define their own unary and binary operators. As a refresher,
unary operators operate on one and only one operand while binary operators operate on exactly two operands. Unary and binary
operators can only be composed of single characters.

#### Defining Unary Operators
Defining unary operators is similar to defining functions: start with the `def` keyword, but then use the `unary` keyword followed by the
character of the unary operator you want to define. Finally follow with a parenthesis-encased space-delimited parameter list and the
expression of the operator.

Using the power of user-defined operators, we can define operators that are usually built-in to most languages:

```
# Logical negation
def unary !(v)
	if v then
		0
	else
		1;

# Unary negation
def unary -(v)
	0-v;
```

Using that last definition of `unary -` allows us to redefine the `abs` function from above as

```
def abs(v)
	if is_positive(v) then
		v
	else
		-v; # Before we had to do 0-v which is what unary - does for us
```

#### Defining Binary Operators
Defining binary operators is just like defining unary ones, except a positive integer appears between the operator and the parameter
list, and we use the `binary` keyword instead of the `unary` keyword. What does the positive integer do? In order to prevent parsing
ambiguities with expressions involving binary operators, pretty much every language (except Lisp which doesn't have this problem due
to its epic S-expressions :sunglasses:) hard-codes a series of _precedences_ for every binary operator to know which one(s) to prioritize.
This has the benefit of matching the order of operations (PEMDAS) from math, so there is no surprising behavior. What does this all mean?
Whenever you define a new binary operator you **must** specify its precedence, which is what that positive integer between the operator
and the parameter list is. The higher the number, the higher the precedence.

```
# We can define the logical "or" operator in our own language! How many languages can do that? Although, this is non-
# short-circuiting.
def binary | 5(LHS RHS)
	if LHS then
		1
	else if RHS then
		1
	else
		0;

# Non short-circuiting logical "and". Notice this has a precedence of 6, so it will take higher precedence over | which
# only has a precedence of 5.
def binary & 6(LHS RHS)
	if !LHS then
		0
	else
		!!RHS; # Convert RHS to a 0/1 with !!

# Back in the "Comparison Operators" section I said only < and > were built into the language, but we can define =, the
# equality operator (NOT the assignment operator). Two numbers are considered equal if neither is greater nor less than
# the other.
def binary = 9 (LHS RHS)
	!(LHS < RHS | LHS > RHS);

# In this example, & has higher precedence over |, so this parses as
#
# (3 = 3.3) | ( (2 + 2 = 5) & (-20 / 2 = 3 - 13) ) | (1 = 1)
#
# rather than
#
# ( (3 = 3.3) | (2 + 2 = 5) ) & ( (-20 / 2 = 3 - 13) | (1 = 1) )
(3 = 3.3) | (2 + 2 = 5) & (-20 / 2 = 3 - 13) | (1 = 1)
``` 


You can find the syntax of Kaleidoscope altogether in the sample file [test/kaleidoscope_input.txt](test/kaleidoscope_input.txt).

## Building From Source
Ensure LLVM is installed on your machine. I've been building this against LLVM version 11.1.0; it might build with newer versions
but I have not tested that. On a Mac with [Homebrew]: `brew install llvm@11`.

The project is controlled by a single Makefile. In the root of this repository, run

```Bash
make
```

The resulting binary is called `kaleidoscope` and lives in the `target/release` directory.
You can also run

```Bash
make debug
```

to get a debug build in the `target/debug` directory, which builds with debug symbols suitable for [LLDB] as well as [Address Sanitizer][AddressSanitizer]
(for getting an informative stacktraces for segfaults) and [Undefined Behavior Sanitizer][UBSan] (for reporting usage of
undefined behavior).

Additionally, there is an [examples](examples) directory that contains self-contained programs that test certain components of the
interpreter. You can build those with

```Bash
make examples
```

The resulting binaries will be found in `target/debug`.

There is also a `make` target for formatting code:
```Bash
make fmt
```

For this target to run successfully you must also have `clang-format` and [`shfmt`](shfmt) installed.

Finally, there is a rudimentary test that tests the lexer and that the interpreter doesn't crash with the aforementioned
[test/kaleidoscope_input.txt](test/kaleidoscope_input.txt):
```Bash
make test
```

[LLVM Tutorial]: https://llvm.org/docs/tutorial/index.html
[Homebrew]: https://brew.sh
[LLDB]: https://lldb.llvm.org
[AddressSanitizer]: https://github.com/google/sanitizers/wiki/AddressSanitizer
[UBSan]: https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html
[shfmt]: https://github.com/mvdan/sh
