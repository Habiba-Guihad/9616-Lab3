# Lab 3 - Mini Shell

CC373 Operating Systems, Alexandria University.

Name: YOUR NAME HERE
ID: YOUR ID HERE

A small Unix shell written in C++. It reads a line, splits it into tokens, builds a command table, and runs it with fork/execvp.

## Files

- `tokenizer.cc`, `tokenizer.h` - turns the typed line into tokens (words, `|`, `<`, `>`, `>>`, `2>`, `>>&`, `&`). Handles single and double quotes, and operators work without spaces (`ls>out`).
- `command.cc`, `command.h` - the command table, `parse()`, `execute()`, built-in commands, signal handling.
- `Makefile` - builds everything into `myshell`.

## Build and run

    make
    ./myshell

`make clean` removes the built files.

## What it supports

- Running programs with arguments (`ls -al`, `echo hello`)
- Redirection: `>` `>>` `<` `2>` `>>&`
- Pipes of any length (`ls | grep a | wc -l`)
- Background jobs with `&` (only allowed at the end of the line)
- Built-ins: `cd [dir]` (no argument goes to HOME) and `exit`
- Ctrl-C does not kill the shell, but it still stops the program that is running
- Wildcards `*` and `?` (bonus), using `glob()`
- Log file: every time a child process ends, a line is written to `myshell.log`. This uses a SIGCHLD handler, which also stops finished background jobs from staying as zombies.

## Error messages

The shell prints a message and goes back to the prompt for:

- unknown command: `myshell: foo: command not found`
- missing input/output file: `myshell: nofile: No such file or directory`
- bad syntax: a pipe or redirection with nothing after it, a pipe with nothing before it, `&` in the middle of a line, two `>` or two `<` in one command, a line with only a redirection
- `cd` to a folder that does not exist

## Notes on the starter code

Some bugs in the given code were fixed:

- `Command()` used `_numberOfSimpleCommands` before setting it, and `_out_error` / `_append` were never initialised.
- `print()` could crash when no error file was set.
- `clear()` used `free()` on objects created with `new`.
- `execute()` called `prompt()` again, so the shell recursed forever and never handled Ctrl-D.

## Known limitations

- Quotes are removed in the tokenizer, so `echo "*"` still expands the star (a real shell would not).
- `~` is not expanded.
- Redirection is applied once per line: `<` to the first command and `>` to the last one.
- No command history, no variables, no `;` or `&&`.
- Hidden files (starting with `.`) are not matched by `*`.

## Testing

Tested by hand inside the shell with the lab handout examples, plus extra cases for errors, empty input, long argument lists, 6-stage pipes and background jobs.
