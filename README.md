# `ifswitch` -- `gcc` plugin to rearrange `switch` statements in C

This is a `gcc` compiler plugin designed to rearrange the AST of certain
`switch` statements into an equivalent form with `if`/`else`.

Licensed under [ISC License](https://www.gnu.org/licenses/license-list.html#ISC).

## Outline of the problem:

A typical `switch` statement in C goes something like this:

```c
switch (value) {
    case 1: {
        // might create a variable in this scope
        something_1();
        printf("you got a 1\n");
        break;
    }

    case TWO:
        something_2();
        printf("you got a 2\n");
        // fall-through

    case THREE:
        something_3();
        printf("you got a 3\n");
        break;

    case 0:
        something_0();
        // fall-through

    default:
        print("you got a %d\n", value);
}
```

Now the above statement will be considered valid by the compiler _only if_ all
the case values are known at compile-time. So `TWO` and `THREE` have to be
`#define`s or `static const` values within the same translation unit, otherwise
`gcc` gives you an error like `case is not constant`.

For example, if `TWO` and `THREE` were defined as:

```c
extern const int TWO;   // defined in other.c
extern const int THREE; // defined in other.c
```

You would get the aforementioned `case constant` error.

## Goal of this plugin

The goal of this plugin is find switch statements like the above example and
transform them as follows:

```c
// rest of your code before the switch is unchanged

int __plugin_tmp = value;
// decide type of __plugin_tmp based on cases

if (1 == __plugin_tmp) goto __plugin_switch_case_1;
else if (TWO == __plugin_tmp) goto __plugin_switch_case_TWO;
else if (THREE == __plugin_tmp) goto __plugin_switch_case_THREE;
else if (0 == __plugin_tmp) goto __plugin_switch_case_0;
else goto __plugin_switch_default;

{ // switch usually adds a scope
    __plugin_switch_case_1: {
        // might create a variable in this scope
        something_1();
        printf("you got a 1\n");
        goto __plugin_switch_end;
    }

    __plugin_switch_case_TWO:
        something_2();
        printf("you got a 2\n");
        // fall-through

    __plugin_switch_case_THREE:
        something_3();
        printf("you got a 3\n");
        goto __plugin_switch_end;

    __plugin_switch_case_0:
        something_0();
        // fall-through

    __plugin_switch_default:
        print("you got a %d\n", __plugin_tmp);
        goto __plugin_switch_end;
}
__plugin_switch_end: ;;

// rest of your code after the switch is unchanged
```

Note that we do _not_ want to allow arbitrary C expressions as `switch` cases --
we just want to allow case constants that are not known at compile time.

## Is this even valid/possible?

This transformation depends on a following details:

- [ ] `gcc` plugins can access the type information of `case` values
- [ ] `gcc` plugins can access the type of the input to `switch`
- [ ] `gcc` plugins can determine if an AST subtree is a value, `const` variable,
  or an arbitrary expression
- [ ] `gcc` plugins allow modifying the AST (like `openmp` or `randstruct`)
- [ ] `gcc` plugins allow modifying the AST _after_ the preprocessor has completed,
  but _before_ raising the `case constant` error
- [ ] the AST rewrite into the `if`-`else` form produces non-horrible assembly when compared 
  to the ideal `switch` statement (check via Godbolt)
- [ ] there are few (hopefully zero) edge cases where this transformation
  breaks invariants (Duff's device or some C++ class where
  `operator=`, `operator==`, and `operator int()` are defined with side-effects)
- [ ] there are few (hopefully zero) edge cases where this transformation
  introduces UB and/or connected security risk

## Why a compiler plugin? Is there no other way?

There might be other ways to handle such incorrect switch statements, but any
method to rearrange these `switch` statements would need to incorporate a C
preprocessor and parser, and any source code transformations would need to
remain valid even if `ifdef`s are mixed within the C source code.

Mixing `ifdef`s` is a quite common occurrence in `switch` statements -- often
times you see handlers for `errno` having a bunch of `ifdef`s (and
fallthroughs!) to allow for different kinds of errno values based on the
operating system.

The best place to handle these switch statements is _after_ the preprocessor has
done its work, so that the focus can be solely on the AST. `gcc` comes in with a
battle-tested C preprocessor and parser, so why not a `gcc` plugin?

## Possible implementation style: via a `pragma`

This would require someone to annotate the necessary `switch` statements with
something like

```c
#pragma ifswitch rearrange
switch(value) {
    // cases
}
```

Advantage: requires human approval -- since the codebase has to be changed, we
will know that the AST was rewritten because it was requested specifically by
the progammer.

Disadvantage: requires manual checks of the code -- you would need to compile
your code, find out this error, then annotate the specific `switch` statement.


## Possible implementation: analyze all and change only if necessary

In this method, the plugin would go through each `switch` statement, and choose
to rearrange only if any of the cases are not known constants.

Advantage: requires no annotation or manual labor -- you can just specify the
plugin in your build script wherever you call `gcc` and it would silently
rewrite the AST where necessary.

Disadvantage: It may be slow to check every `case` in every `switch`. Also, it
is known how much type information is present for the plugin to use -- for
example, does the plugin know if the `case` is a `#define`, `static const`,
`extern const`, or (ouch) an arbitrary expression?

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
