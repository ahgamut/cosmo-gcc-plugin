# `ifswitch` -- transform `switch` statements to `if` statements

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
        printf("you got a %d\n", value);
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

## What does this plugin do?

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
        printf("you got a %d\n", __plugin_tmp);
        goto __plugin_switch_end;
}
__plugin_switch_end: ;;

// rest of your code after the switch is unchanged
```

Note that we do _not_ want to allow arbitrary C expressions as `switch` cases,
not even case ranges, -- we just want to allow case constants that are not known
at compile time.

## Is this even valid/possible?

This transformation depends on the following details:

- [x] `gcc` plugins can access the type information of `case` values -- they're
  all just integer values (or `INTEGER_CST`s in AST-speak)
- [x] `gcc` plugins can access the type of the input to `switch` -- this ends up
  not mattering as much, because I use `save_expr` instead of creating a
  separate temporary variable `__plugin_tmp`
- [x] `gcc` plugins can determine if an AST subtree is a value, `const` variable,
  or an arbitrary expression -- yes, but sometimes locations are wacky,
  especially if we have multiline comments.
- [x] `gcc` plugins allow modifying the AST (like `openmp` or `randstruct`)
- [ ] `gcc` plugins allow modifying the AST _after_ the preprocessor has completed,
  but _before_ raising the `case constant` error to the user -- this is not
  possible
- [ ] there are hopefully zero edge cases where this transformation
  breaks invariants (Duff's device or some whack C++ class where
  `operator=`, `operator==`, and `operator int()` are defined with
  side-effects) -- `save_expr` should be okay, remains to be tested

## Current design: analyze all and change only if necessary

In this method, the plugin would go through each `switch` statement, and choose
to rearrange only if any of the cases are not known constants.

Advantage: requires no annotation or manual labor -- you can just specify the
plugin in your build script wherever you call `gcc` and it would silently
rewrite the AST where necessary.

Disadvantage: It _may_ be slow to check every `case` in every `switch` (so far
it hasn't been an issue).

## Alternative design: via a `pragma`/attribute

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
Also toggling certain behaviors within the pragma is a bit weird.

## Alternative design: intercept the error and modify necessary AST

In this method, the plugin would activate only if `gcc` was raising a `case
constant` error with respect to a particular `switch` statement.

Advantage: requires no manual labor, and we check/modify only those `switch`
statements which actually need to be modified.

Disadvantage: this is (likely) not possible to do -- the issue is that the AST
after a parse error is incomplete, so any modifications are not possible. There
is also a fear of the `case constant` error being raised due to some unrelated
issue, and the plugin making things worse. ** Edit: ** I ended up [patching
`gcc`][gccpatch] to do pretty much this -- instead of using macros, I modify the
AST with the necessary `__tmpcosmo_` value (after confirming it exists), and
prevent `gcc` from raising the `case constant` error. Fixing the AST later is
done as usual.

[gccpatch]: https://github.com/ahgamut/gcc/tree/portcosmo-11.2
