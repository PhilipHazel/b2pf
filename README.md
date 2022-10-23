# B2PF - A library for converting characters from base to presentation forms.

The B2PF library is a set of C functions that process character strings in
languages such as Arabic, where the characters change their shapes depending on
their neighbours within words. B2PF reads input encoded as Unicode base
characters. It converts characters that form words to the appropriate
*presentation forms* according to a set of externally-supplied rules. A set of
rules for Arabic script are supplied.

B2PF can process UTF-8, UTF-16, or UTF-32 input. It can handle left-to-right or
right-to-left input and convert the output if required.

## Documentation

You can read the B2PF documentation from the
[here](https://philiphazel.github.io/b2pf/doc/html/index.html).
