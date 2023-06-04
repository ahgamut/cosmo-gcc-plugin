# `portcosmo` - `gcc` plugin to help port software to Cosmopolitan Libc

This is a `gcc` plugin written to ease porting C software to Cosmopolitan Libc.
The general idea is to reduce manually changing the source code of any external
software when attempting to build it with Cosmopolitan Libc -- ideally, you
would need to customize the build process, but make zero changes to the source
code.

Licensed under [ISC License](https://www.gnu.org/licenses/license-list.html#ISC).

**Note**: this plugin has not yet been fully tested -- please check the compiled
`.o` file, generated ASM, or errors in your test suite to confirm the
correctness of the transformations. When in doubt, transform the code manually.
See the [Counterexamples](#Counterexamples) section for more details.

** Edit: ** I ended up [patching `gcc`][gccpatch] with the code from this
plugin. The patched `gcc` does a lot less work than this plugin (and avoids a
lot of the counterexamples) because I avoid using the macro hack and just patch
the AST before the parser complains.

## How to use this plugin?

1. Install the necessary `gcc` plugin headers (you need `gcc` to be able to use
   its plugin architecture)
2. Clone this repository and run `make`
3. Create a small shell script that uses `/usr/bin/gcc` with this plugin (ie add
   `-O2 -fplugin=/location/of/portcosmo.so -include /location/of/tmpconst.h`)
    and use that as `CC` when building software.

As for now you will need to use [this
branch](https://github.com/ahgamut/cosmopolitan/tree/symbolic-macro) where I've
been trying to ensure I change as little of Cosmopolitan Libc as possible in
order to make this work.

And it does work! [This
branch](https://github.com/ahgamut/cpython/tree/py311-custom) of CPython
3.11.0rc1 builds with Cosmopolitan Libc, and I don't had to modify any `switch`
statements.

## How does it work?

Cosmopolitan Libc contains system-level constants (for example, errno constants
like `SIGABRT`) defined as follows:

```c
extern const int SIGABRT;
#define SIGABRT ACTUALLY(SIGABRT)
```

This plugin activates upon finding a ` ACTUALLY(` (note the space) within a
defined macro, and (re-)defines `ACTUALLY` as follows:

```c
#define ACTUALLY(X) __tmpcosmo_##X
```

and records the location in the source file every time a macro containing
`ACTUALLY(` is used. In `tmpconst.h`, there is a huge list of constants starting
with the `__tmpcosmo_` prefix.

After every (valid) macro usage has been recorded, this plugin walks through the
entire AST of the source file to find each usage, and substitutes the
appropriate `extern` variable name in the location where the macro was used. It
does so via the below two components:

* `ifswitch` -- rearrange `switch` statements if the case labels would otherwise
  raise the `case label is not constant` error.
* `initstruct` -- update definitions of variables, `struct`s, and arrays if
  their initialization would otherwise raise the `initializer element is not
  constant` error (can handle `static` and global variables).

The plugin errors out if the `ACTUALLY` macro was improperly used, or if it is
unable to confirm all the macro usage records were substituted successfully. At
the end of compilation, the plugin provides a note of how many substitutions
were made when compiling the file.

## Why a compiler plugin?

There might be other ways to check for such incorrect statements, but any
method to rearrange these `switch` statements would need to incorporate a C
preprocessor and parser, and any source code transformations would need to
remain valid even if `ifdef`s are mixed within the C source code.

Mixing `ifdef`s is a quite common occurrence in `switch` statements -- often
times you see handlers for `errno` having a bunch of `ifdef`s (and
fallthroughs!) to allow for different kinds of errno values based on the
operating system.

The best place to handle these statements is _after_ the preprocessor has done
its work, so that the focus can be solely on the AST. `gcc` comes in with a
battle-tested C preprocessor, parser, decent optimizations, and plugin support,
so why not a `gcc` plugin?

## Counterexamples

While this plugin can traverse through the code AST and modify almost all uses
of the macro, there are a few cases where it may not be able to do so:

* Using `gcc -O0` i.e. if you disable all optimizations, then `gcc` will not
  perform constant-folding and error out with `case label is not constant` with
  some source code like

```
case __tmpcosmo_SIGABRT:
```

This can likely be fixed, it's just a matter of enabling the right optimization
flag in `gcc`. Better yet: we can figure out how to use `__tmpcosmo_SIGABRT` as
a macro that can be defined during runtime, instead of a `static const int` in
`tmpconst.h`, which would circumvent this problem. ** Edit: ** I ended up
[patching `gcc`][gccpatch] with the code from this plugin, so this problem is avoided.

* `case` labels with ranges, something like:

```c
case SIGABRT .. 0:
```

Yes, I know it's possible to make this work, but I haven't seen any real-life C
code that does something like this yet.

* constant-folding algebra:

```c
static const int e = SIGABRT;
/* few lines later... */
func(e);
```

Under `gcc`'s optimization flags, `e` will be constant-folded, and its value
will be used everywhere instead. The plugin has not recorded all the locations
where `e` could have been used, so it just bails out when seeing a declaration
like this. ** Edit: ** I ended up
[patching `gcc`][gccpatch] with the code from this plugin, so this problem is avoided.

```c
int x = SIGABRT+42;
if(j < SIGABRT+42)
case SIGABRT+42:
for(int i=SIGABRT-1; i < 0; ++i)
```

Under `gcc`'s optimization flags, all of the above statements will have been
constant-folded, and even though the plugins has recorded where the macro was
used, it does not know what expression was simplified, so it bails out if it was
unable to substitute a constant in any expression. ** Edit: ** I ended up
[patching `gcc`][gccpatch] with the code from this plugin, so this problem is avoided.

* magical things like Duff's device -- I don't know if any C code uses Duff's
  device with `SIGABRT`, would be fun to find out.  ** Edit: ** I ended up
[patching `gcc`][gccpatch] with the code from this plugin, so this problem is avoided.

* substituting the incorrect location due to a `bad` pick of constant: Suppose
  we have some code which uses a lot of integer constants, and some of them *are
  on the same line* as when one of our macro substitutions was recorded, then
  the plugin will likely substitute the constant at the wrong location. See the
  below example:

```c
/* suppose tmpconst.h has the below value */
static int __tmpcosmo_SIGABRT = -961;

/* and your code has something like */
func(-961, SIGABRT);

/* the macro will modify it to */
func(-961, __tmpcosmo_SIGABRT);
/* and record the location of the modification */

/* but gcc will constant-fold it to */
func(-961, -961);

/* the AST will be INCORRECTLY transformed into */
func(SIGABRT, -961);

/* whereas the second param should actually be transformed */
func(-961, SIGABRT);
```

It might be possible to fix this via a hash-table of some sort, because we can
just check the function call/expression at a marked location to confirm that it
does *not* have the constant we just substituted anymore(ie our substitution
actually fixed the macro use and some other constant in the source code). 

This can also be fixed if we had more precise location checking, at present, if
your source code has a function call like 

```c
func(27, __tmpcosmo_SIGABRT);
```

In terms of line information, we only know that the `CALL_EXPR` with `func`
starts on line 42 (and also its end sometimes) -- we do not know the location of
the the individual parameters `27` and `-961`, which would be useful to match
with the location we have saved from when the macro was used. ** Edit: ** I
ended up [patching `gcc`][gccpatch] with the code from this plugin, so this
problem is avoided in *most* situations (I haven't found an example of this
problem in real-life code yet). It can still happen if you're
initializing a struct or writing a `switch` case with the clashing values, but
my current belief is that the latter is quite rare (a `switch` whose options
include both errno constants and other unrelated negative values), and the
former is still uncommon, and would be caught by a simple test. Either way, the
fix is to use different constants, or do the AST patching by hand.

## References:

- [History of C](https://en.cppreference.com/w/c/language/history)
- [C99 `switch` constraints and semantics](https://www.open-std.org/jtc1/sc22/wg14/www/C99RationaleV5.10.pdf) -- see page 92, $\S 6.8.4.2$
- [C11 `switch` constraints and semantics](https://open-std.org/JTC1/SC22/WG14/www/docs/n1570.pdf) -- see page 149, $\S 6.8.4.2$
- [C17 final draft `switch` constraints and semantics](https://files.lhmouse.com/standards/ISO%20C%20N2176.pdf) -- see page 108, $\S 6.8.4.2$
- [Assert Rewriting in GCC](https://jongy.github.io/2020/04/25/gcc-assert-introspect.html)
- [An Introduction to GCC and GCC's plugins](https://gabrieleserra.ml/blog/2020-08-27-an-introduction-to-gcc-and-gccs-plugins.html)
- [LWN: Randomizing Structure Layout](https://lwn.net/Articles/722293/)
- Source code of the
  [`randstruct`](https://github.com/torvalds/linux/blob/d37aa2efc89b387cda93bf15317883519683d435/scripts/gcc-plugins/randomize_layout_plugin.c) plugin used in the Linux kernel -- this is to understand how much can be done at the GCC plugin level.
- [GCC OpenMP Runtime Wiki](https://gcc.gnu.org/wiki/openmp) -- need to
  understand how `pragma`s can be used to alert a plugin


[gccpatch]: https://github.com/ahgamut/gcc/tree/portcosmo-11.2
