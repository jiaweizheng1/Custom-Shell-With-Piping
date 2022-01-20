# SGREP: Simple Grep

## Summary

This program, `sgrep`, can print the lines of one of more files which match one
or more specific string patterns. It also supports printing the line number of
matching lines, as well as coloring the matched pattern within each line.

## Implementation

The implementation of this program follows two distinct steps:

1. Parsing the options given as program parameters
2. Performing the matching between the contents of the specified files and the
   specified string patterns

### Parsing options

At the beginning of its execution, `sgrep` parses the option given as program
parameters (i.e., through `argv[]`) and build a "configuration" data structure
called `struct config`. This configuration data structure is a single object
containing two kinds of information information.

First, we have two fields which are simple integer flags and reflect the options
`-n` (for enabling the printing of line numbers in the output) and `-c` (for
color printing the matching pattern in each output line). These fields are set
to 1 if the corresponding option is enabled, or 0 otherwise.

Second, we have two pointers to a singly linked list in order to keep track of
the specified string patterns. The choice of a singly linked list allows for an
arbitrary number of patterns to be specified, while maintaining a low memory
footprint and fast access.

The first pointer, `p`, is meant to be the head of the list, which later allows
the list to be scanned in the exact order that patterns were specified (when
doing the pattern matching). The other pointer, `p_tail`, is meant to point on
the last item of the singly linked list, which allows new pattern to be easily
appended in $$O(1)$$.

### Pattern matching

Once the configuration structure is built, `sgrep` can now perform the actual
pattern matching with the help of three functions.

Function `grep_files()` goes through the rest of the program parameters (still
through `argv[]`) which specify the list of files to process. Using a loop, each
file is opened and further analyzed by calling `grep_lines()`.

Function `grep_lines()` analyzes all the files of a given file. It reads each
line in a buffer and calls `grep_line()` to detect if that particular line
matches one of the string patterns.

Function `grep_line()` performs a typical singly linked list traversal through
all the string patterns pointed by `p` in the configuration structure. If a
string pattern appears within the current line, then this line should be
printed. The printing is further specialized based on the flag fields contained
in the configuration structure (i.e., line number and coloring).

### Note about the memory management

The only objects that are allocated dynamically are the string patterns, since
there can be an arbitrary amount of them. You'll notice that they are never
properly deallocated; this is justified by the fact that these objects are
required until the program exits, which will automatically free the heap anyway.
There are therefore no memory leaks per se.

## License

This work is distributed under the [GNU All-Permissive
License](https://spdx.org/licenses/FSFAP.html).

Copyright 2022, Joël Porquet-Lupine
