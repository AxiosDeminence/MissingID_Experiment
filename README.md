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

## Description

Originally meant to provide a quick way to parse csv/tsv files (and other
character delimited files) to check for missing IDs throughout a numberof files
without the overhead of pandas and other Python libraries. Gradually became an
exercise on learning how to use the [glib's
argp](https://www.gnu.org/software/libc/manual/html_node/Argp.html) and also
learning how linkage works in the gcc compiler.

Given the above [problem description](problem-description), the intended use of
the program was the following: 
1. Save separate queries into well-formed csv/tsv's
2. Run the executables with the input files
3. Receive the lowest missing ID
4. Query the missing ID and take note of the character attribution
5. Query the character attribution and save that query into a well-formed 
   csv/tsv
6. Repeat steps 2 through 5.
