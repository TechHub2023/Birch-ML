# Birch Standard Library

Provides the basic functionality needed by most Birch programs, such as standard math, linear algebra, probability distributions, and I/O.


## License

Birch is open source software.

It is licensed under the Apache License, Version 2.0 (the "License"); you may not use it except in compliance with the License. You may obtain a copy of the License at <http://www.apache.org/licenses/LICENSE-2.0>.


## Installation

To build and install, use:

    birch build
    birch install

See the `DOCS.md` file for reference documentation.


## Developer notes

The standard library include parsers for several file formats that are contained in `*.y` files. The `lemon` parser generator is used to build C sources for these. These C sources are included in the repository so that users of the standard library need not build them themselves. For developers, however, after modifications to any `*.y` files, run `make_parser.sh` manually to rebuild the C sources.


## Version history

### v0.0.0

* Pre-release.
