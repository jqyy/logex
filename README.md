# Logex

This program parses a logic expression and prints its truth table.

## Syntax

For EBNF notation, see the source.

- Identifier: ``[A-Z]`` (single upper letter).
- Constant: ``[01]`` (**false** and **true**).
- Operators: ``[&*]`` (AND), ``[|+]`` (OR, optional), ``^`` (XOR), ``[~!]``(NOT).
- Priority: NOT > XOR = OR > AND, all left associative.

## Notes

- Identifiers are ordered as they appear in the input, **NOT** alphabetically.
- Future version will be able to simplify logic expressions.
