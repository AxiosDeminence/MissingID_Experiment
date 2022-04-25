# Missing ID Checker - CSV/TSV Parser

## Problem Description

One of the roleplay communities that I am a part of integrates dice rolling in
order to have some random elements similar to traditional tabletop RPGs. The
database for the website we roll dice on has data as early as 2014, but the
maintainer of the service has been inactive and unresponsive to any requests or
inquiries about the status of the service. This has been a cause of concern
about the potential loss of rolls, especially since rolls and their linked IDs
have been used to keep a central record of information about the community's
history. However, we only had immediate access to the following relevant
information:
- Roll attributes (specifically the ID and attributed character)
- Up to the past 200 rolls
- All rolls attributed to a character
- The singular roll with a specific ID

## C Executable Solution

Originally meant to provide a quick way to parse csv/tsv files (and other
character delimited files) to check for missing IDs throughout a numberof files
without the overhead of pandas and other Python libraries. Gradually became an
exercise on learning how to use the [glib's
argp](https://www.gnu.org/software/libc/manual/html_node/Argp.html) and also
learning how linkage works in the gcc compiler.

Given the above [problem description](problem-description), the intended use of
the ./main executable was the following: 
1. Save separate queries into well-formed csv/tsv's
2. Run the executables with the input files
3. Receive the lowest missing ID
4. Query the missing ID and take note of the character attribution
5. Query the character attribution and save that query into a well-formed 
   csv/tsv
6. Repeat steps 2 through 5.

The C files is ANSI-compliant with the exception of glib's argp. This should
mean that regardless of the compiler options, as long as they are ISO C89/C90
compliant.

### Usage

* `-c, --columns [int]`: Zero-indexed column corresponding to the IDs. Default column 0.
* `--headers [bool]`: Ignore headers in files. Default false (still functions if false even with headers).
* `-i, --input [file]`: Input files to search through
* `-o, --output [file]`: Obsolete. Will print out the missing ID to the terminal.
* `-q, --quote [char]`: Quote character.
* `-d, --delimiter [char]`: Delimiter character.
* `-?, --help, --usage`: Prints a help message.

See ./main --usage for more details.

## NodeJS Crawler

I ended up deciding that it would be a good idea to find a way to automate
querying all of the rolls into a DSV rather than manually querying the IDs.
As such, I ended up learning how to use NodeJS' Node-API/N-API in order to
import the existing C code as a Node-API Native Addon and using the experimental
fetch API in order to query the webserver. Afterwards, I used cheerios to parse
the queries and automatically build the DSV.

### Usage:
* `-i, --input [file]`: Present files to check for the next missing ID.
* `-o, --output [path]`: File (or directory) to output the DSV(s) to. See `--separate-character-files` for more detail.
* `-q, --quote [character]`: Quote character.
* `-d, --delimiter [character]`: Delimiter character.
* `--dir, --base-dir [directory]`: Base directory to save input and output files to.
* `--separate-character-files [boolean]`: If true, each query will be saved in a separate file with the character name. `--output` should be a directory.

## Build Process
After cloning the repository, running the Makefile and performing node-gyp rebuild
should allow for src/module.js to be run.

## Dependencies
* NodeJS v17.9.0
* node-gyp
* Various modules in the git submodule such as:
  * Cheerios v1.0.0-rc.10
  * Minimist v1.2.6
  * LibCSV (custom fork fixing)