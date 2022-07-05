# Pscript

[![CMake](https://github.com/NotAPenguin0/pscript/actions/workflows/cmake.yml/badge.svg?branch=master)](https://github.com/NotAPenguin0/pscript/actions/workflows/cmake.yml)

![lines](https://img.shields.io/tokei/lines/github/NotApenguin0/pscript)

Custom programming language built for scripting plugins in my engine.

## Documentation

### 0. Hello, World!

```cpp
import std.io;
std.io.print("Hello, World!");
```

### 1. Basic syntax

Variables are defined using the `let` keyword. An initializer is required, from which the
type of the variable is inferred. Once declared, a variable cannot change type.

```rust
let x = 1;
let y = "abc";
```

There is an exception to this rule. Re-declaring a variable with a new type will shadow
the old variable in the current scope. 

```rust
let x = 1;
let x = "abc"; // x is now a string.
```

Basic control sequences are supported using `if`, `while`, `for` and `else` with similar 
syntax to C and C++.

```cpp
if (condition) {
    // statements
} else if (other_condition) {
    // other statements
} else {
    // final statements
}
```

```cpp
while(true) {
    // do something
}
```

```rust
for(let i = 0; i < 10; ++i) {
    // do something
}
```

### 2. Types

The following builtin types are available:

- `int`
- `uint`
- `float`
- `bool`
- `str`
- `list`
- `any`
- `void`

`any` refers to any posible type and is useful for passing generic arguments or
external types around (see later). A string is created by wrapping text inside 
quotes (`"abc"`). A list can be created by using square braces:

```rust
let my_list = [1, 2, 3];
let empty_list = [];

empty_list.append(4);
```

Additionally, new types can be defined using structs (see later).

### 3. Functions

A function is declared using the `fn` keyword, followed by its name, parameters and a
return type.

```rust
fn foo() -> int {
    // execute foo()
}

fn bar(a: int, b: any) -> void {
    // do something with parameters a and b.
}
```

A function can then easily be called using standard function call syntax:
```rust
bar(a, "xyz"); // since the type of b was 'any', we can pass any type.
```

### 4. Structures

A struct allows for creating new types that can be used anywhere after the struct was declared.
A struct declaration starts with the `struct` keyword and is followed by a block with member
declarations. Note that like in C and C++, the struct declaration must end with a semicolon.

```rust
struct MyStruct {
    x: int = 0;
    y: str = "abc";
};
```

Similar to variables, the initializer for struct members is required and must be convertible
to the member type. An instance of a struct can be created as follows:

```rust
let s = MyStruct { 42, "The meaning of life" };
let s2 = MyStruct { 42 }; // 'y' is initialized to "abc"
```

The initializers are assigned to the members in order. You can pass fewer initializers to use
the default values for those members. To access a struct member, use the `->` operator.

```rust
let s_x = s->x;
```

### 5. Type casting

Sometimes types do not match exactly. When this happens, the interpreter will try to cast
to the type the called function, variable or struct initializer expects. If this is not
possible, a `TypeError` will be reported. You can also cast between compatible types manually
by using the same syntax as struct construction:

```rust
let x = float { 5 };
let y = uint { 6 };
```

Because this syntax for creating unsigned integers is cumbersome, a *literal suffix* `u`
is also provided that does the same thing.

```rust
let x = 6u;
```

### 6. Modules

Importing code from other files (like the standard library) is made possible through
*modules*. A module can be imported using the `import` statement.

```cpp
import std.io;
```

This will look in the provided module path (by default only includes the standard
`pscript_modules/` folder) for a file `std/io.ps` and import it. After the import, 
functions, types and variables from the imported file can be accessed by prefixing
their names with the module name.

```cpp
import std.io;

std.io.print("Hello, World!");
```

### 7. External objects

Through the C++ API, external functions and variables can be accessed inside a PScript 
program. To do this, the function or variable must be declared using the `extern` 
keyword.

```rust
extern fn foo(x: int) -> str;
extern let bar: int;
```

After that, hook up the names to your C++ objects using the `ps::extern_library` class.

```cpp
ps::string_type foo(int x) {
    return ps::string_type { std::to_string(x) };
}

int bar = 42;

ps::context ctx(memsize);

ps::extern_library lib {};
lib.add_function(ctx, "foo", &foo);
lib.add_variable("bar", &bar);

ps::execution_context exec {};
exec.externs = &lib;

ctx.execute(my_script, lib);
```

These external functions and variables can now be used like normal PScript objects.

### 8. Reference types

Sometimes it is useful to pass objects to functions by reference. This means no copy is
made, and changes to the parameter are also visible to the caller. By default, lists,
strings and structures are always reference types. To create a reference to another
type, prefix its name using an ampersand `&`.

```rust
fn foo(x: int&, a: int, b: int) -> void {
    a = a + 1;
    x = a + b;
}

let result = 0;
let a = 10;
let b = 9;
foo(result, a, b); // will set result to 20, but leave a unmodified.
```

### 9. Standard library reference

WIP. for now, please refer to the source code in `modules/std`.

### 10. C++ API

PScript programs can be executed through the C++ API implemented in this repository.
Everything is available with a single `#include <pscript/context.hpp>`. To execute a
script, first load it into a `ps::script` object.

```cpp
ps::script source { "let x = 0; let y = 10;", ctx };
```

To run scripts, you must create a `ps::context` object. This object signifies a single 
execution context. Variables created in scripts are created in their context, imported
modules are imported for the entire context, etc. Each context has a memory pool, whose
size must be specified at creation (in bytes).

```cpp
static constexpr std::size_t mem_size = 1024 * 1024; // 1 MiB of memory.
ps::context ctx { mem_size };
```

To execute a script on a context, simply call `context.execute(script);`. This function
has an additional optional parameter of type `ps::execution_context`. This parameter
allows specifying various options to control execution.

- Control standard input and output streams by setting `in`, `out` and `err`.
- Provide an extern library by building a chain of `ps::extern_library` objects (see
  also the provided `ps::extern_library_chain_builder`). This chain structure allows
  linking multiple extern libraries to the same context.
- Provide additional paths to search for modules by adding paths to `module_paths`.
  Note that this also allows removing the path to the standard `pscript_modules/` folder.

### 11. Advanced functionality

#### a) Variadics

Functions can be made to accept any number of arguments by passing in a variadic parameter.
This parameter must always be the last parameter declared. In the function call, the
variable will behave as if it is a `list<any>`.

```rust
fn print_all(x...) {
  for (let i = 0; i < x.size(); ++i) {
    std.io.print(x[i]);
  }
}

print_all(1, 2.2, 3, "abc");
```

As you can see, a variadic parameter can store different types. Variadics can be passed around to other functions or constructors
by expanding them.

```rust
fn printf(fmt: str, args...) {
	// pass all elements of args... through to the builtin format() function
	std.io.print(fmt.format(args...));
}
```
