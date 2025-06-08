# WitsShell

A custom Unix shell implementation written in C that provides basic shell functionality including command execution, built-in commands, path management, I/O redirection, and parallel command execution.

## Features

### Built-in Commands
- **`cd <directory>`** - Change current working directory
- **`path [directories...]`** - Set executable search paths
- **`exit`** - Exit the shell (must be called without arguments)

### Advanced Features
- **I/O Redirection** - Redirect command output to files using `>`
- **Parallel Commands** - Execute multiple commands concurrently using `&` separator
- **Environment Variables** - Support for environment variable expansion using `$VARIABLE`
- **Quoted Strings** - Handle arguments with spaces using double quotes
- **Batch Mode** - Execute commands from script files
- **Interactive Mode** - Standard shell prompt interface

## Compilation

```bash
gcc -o witsshell witsshell.c
```

## Usage

### Interactive Mode
Run the shell interactively:
```bash
./witsshell
```
You'll see the `witsshell>` prompt where you can enter commands.

### Batch Mode
Execute commands from a file:
```bash
./witsshell script.txt
```

## Command Examples

### Basic Commands
```bash
witsshell> ls -la
witsshell> cat file.txt
witsshell> pwd
```

### Built-in Commands
```bash
# Change directory
witsshell> cd /home/user

# Set executable paths
witsshell> path /bin /usr/bin /usr/local/bin

# Clear all paths
witsshell> path

# Exit shell
witsshell> exit
```

### I/O Redirection
```bash
# Redirect output to file
witsshell> ls -la > output.txt
witsshell> echo "Hello World" > greeting.txt
```

### Environment Variables
```bash
# Use environment variables
witsshell> echo $HOME
witsshell> ls $HOME
```

### Quoted Arguments
```bash
# Handle arguments with spaces
witsshell> echo "This is a quoted string"
witsshell> mkdir "My Documents"
```

### Parallel Commands
```bash
# Execute commands in parallel
witsshell> sleep 5 & echo "Hello" & ls
```

## Technical Details

### Path Management
- Default path is set to `/bin/` on startup
- Use the `path` command to modify executable search paths
- Commands are searched in the order paths were specified
- If no paths are set, external commands cannot be executed

### Error Handling
- All errors output the standard message: "An error has occurred\n"
- Errors are written to stderr
- Invalid commands, file operations, and syntax errors are handled gracefully

### Process Management
- External commands are executed in child processes using `fork()` and `execv()`
- Parent process waits for child completion unless running in parallel
- Proper cleanup of zombie processes in parallel execution

### Memory Management
- Dynamic memory allocation for paths and command arguments
- Proper cleanup of allocated memory on exit
- Handles memory allocation failures gracefully

## Implementation Notes

### Redirection Rules
- Only supports output redirection (`>`)
- Redirects both stdout and stderr to the specified file
- File is created with permissions 0644
- Multiple `>` symbols in a command result in an error
- Redirection must have a valid filename

### Parallel Execution
- Commands separated by `&` run concurrently
- Each command runs in its own child process
- Shell waits for all parallel commands to complete before accepting new input

### Environment Variable Expansion
- Variables prefixed with `$` are expanded to their values
- Undefined variables are treated as empty strings
- Expansion occurs during command tokenization

## Error Conditions

The shell will output "An error has occurred\n" for:
- Invalid file operations
- Command not found
- Memory allocation failures
- Invalid syntax (malformed redirection, etc.)
- Built-in command errors (invalid directory for `cd`, etc.)
- Exit command called with arguments

## Limitations

- No support for input redirection (`<`)
- No piping between commands
- No background process management beyond parallel execution
- No command history or tab completion
- No support for complex quoting or escaping beyond basic double quotes
- No globbing or wildcard expansion

## File Structure

```
witsshell.c          # Main source file containing all implementation
README.md            # This documentation file
```

## License

This project was created as an educational exercise. Feel free to use and modify as needed.

---

*Created as part of a systems programming exercise to understand Unix shell internals and process management.*
