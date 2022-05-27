# pscript specification

## Variable declarations

```pscript
let x = 5;
let y = 9.6;
```

Initializer required, type inferred from it.

```pscript
let x = 5;
let x = 9.6;
```

Variable shadowing is allowed, doing so creates a new variable with the new type.

```pscript
let x = 5;
x = "abc";
```

Changing the type of a variable must be done by shadowing it. 
This means that the code above is not allowed.

## Variable naming
    - names must start with a letter
    - names can consist of letters, numbers and underscores.