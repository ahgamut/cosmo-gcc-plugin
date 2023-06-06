# `initstruct` -- initialize data with proper constants

## Outline of the problem

C software libraries typically store important values like `errno` constants in
`static` variables, and in some cases a wrapper `struct` containing the
necessary constant. You can see it here in Python's [`faulthandler.c`][pyfault],
but let's take a simple example:

```c
struct toy {
    int status;
    int value;
};

/* in global scope */
static int errvalues[] = {SIGABRT, SIGILL, SIGFPE};
struct toy t1 = {.status = SIGABRT, .value = 24};
static struct toy t2 = {.status = SIGABRT, .value = 25};
static struct toy box[] = {
    {.status = SIGABRT, .value = 27},
    {.status = SIGABRT, .value = 28},
    {.value = 29, .status = SIGABRT},
};

int main() {
    static int status = SIGILL;
    struct toy m1 = {.status = SIGABRT, .value = 24};
    static struct toy m2 = {.status = SIGABRT, .value = 25};
    return 0;
}
```

Now, the above code will only compile correctly if `SIGABRT`, `SIGILL` and
`SIGFPE` are compile-time constants. But suppose these values are not known at
compile-time, is it possible to initialize the `struct`s so that it uses the
right values when the program is running?

## What does this plugin do?

The goal of this plugin is to append a one-time check that initializes these
declared structs with the right values. This is what happens to the above code:

```c
struct toy {
    int status;
    int value;
};

/* in global scope */
static int errvalues[] = {__tmpcosmo_SIGABRT, __tmpcosmo_SIGILL, __tmpcosmo_SIGFPE};
__attribute__((constructor)) __hidden_ctor1() {
  errvalues[0] = SIGABRT;
  errvalues[1] = SIGILL;
  errvalues[2] = SIGFPE;
}
struct toy t1 = {.status = __tmpcosmo_SIGABRT, .value = 24};
__attribute__((constructor)) __hidden_ctor2() {
  t1.status = SIGABRT;
}
static struct toy t2 = {.status = __tmpcosmo_SIGABRT, .value = 25};
__attribute__((constructor)) __hidden_ctor3() {
  t2.status = SIGABRT;
}
static struct toy box[] = {
    {.status = __tmpcosmo_SIGABRT, .value = 27},
    {.status = __tmpcosmo_SIGABRT, .value = 28},
    {.value = 29, .status = __tmpcosmo_SIGABRT},
};
__attribute__((constructor)) __hidden_ctor4() {
  box[0].status = SIGABRT;
  box[1].status = SIGABRT;
  box[2].status = SIGABRT;
}

int main() {
    static int status = __tmpcosmo_SIGILL;
    static uint8_t __chk1 = 0;
    if(__chk1 == 0) {
        status = SIGILL;
        __chk1 = 1;
    }
    /* for local structs, it just modifies the initializer */
    struct toy m1 = {.status = SIGABRT, .value = 24};
    static struct toy m2 = {.status = __tmpcosmo_SIGABRT, .value = 25};
    static uint8_t __chk2 = 0;
    if(__chk2 == 0) {
        m2.status = SIGABRT;
        __chk2 = 1;
    }
    return 0;
}
```

in the global scope, we have `__attribute__((constructor))` with the highest
priority, that run exactly once upon program startup. Within local function
scope, we declare a temporary variable to check for the first-time
initialization, that has , and then copy (or `__builtin_memcpy`) the values we'd
like to the target. This should work even for nested `static` structs,
struct-of-arrays, array-of-structs, etc.

### What about `static const`?

The plugin will error out if attempting to modify a `static const int` or
similar integral primitive type, and raise a warning for `static const struct`
or similar compound type.
 
[pyfault]: https://github.com/ahgamut/cpython/blob/master/Modules/faulthandler.c#L66
