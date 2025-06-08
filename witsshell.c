#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

char **paths;
int path_count;

/***************************************************************
****************************************************************
NB: ^^^^ FUNCTION TO FIND EXECUTABLE PATHS ^^^^
****************************************************************
****************************************************************/
char *find_executable(char *command) {
    //IF NO PATHS ARE SET, RETURN NULL
    if (paths == NULL || path_count == 0){
        return NULL;
    }

    char *full_path = malloc(4096);
    if (full_path == NULL) {
        char error_message[30] = "An error has occurred\n" ;
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }

    for (int i = 0; i < path_count; i++) {
        strcpy(full_path, paths[i]);
        strcat(full_path, "/");
        strcat(full_path, command);

        if (access(full_path, X_OK) == 0) {
            return full_path;
        }
    }

    free(full_path);
    return NULL;
}

/***************************************************************
****************************************************************
NB: ^^^^ FUNCTION TO HANDLE THE CD COMMAND ^^^^
****************************************************************
****************************************************************/
void handle_cd(char *path) {
    if (chdir(path) != 0) {
        char error_message[30] = "An error has occurred\n" ;
        write(STDERR_FILENO, error_message, strlen(error_message));
    }
}

/***************************************************************
****************************************************************
NB: ^^^^ FUNCTION TO UPDATE THE PATH LIST ^^^^
****************************************************************
****************************************************************/
void update_path(char *new_paths[], int new_path_count) {
    //free previously stored paths
    for (int i = 0; i < path_count; i++) {
        free(paths[i]);
    }
    free(paths);

    // If no new paths are provided, set paths to NULL and update path_count
    if (new_path_count == 0) {
        paths = NULL;
        path_count = 0;
        return;
    }

    // Allocate memory for the new paths
    paths = malloc(new_path_count * sizeof(char *));
    for (int i = 0; i < new_path_count; i++) {
        paths[i] = strdup(new_paths[i]);
    }

    //store the new paths
    for (int i = 0; i < new_path_count; i++) {
        paths[i] = strdup(new_paths[i]);
        if (paths[i] == NULL) {
            char error_message[30] = "An error has occurred\n" ;
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        }
    }

    path_count = new_path_count;
}

/***************************************************************
****************************************************************
NB: ^^^^ FUNCTION TO HANDLE THE PATH COMMAND ^^^^
****************************************************************
****************************************************************/
void handle_path(char *new_paths[]) {
    int new_path_count = 0;
    while (new_paths[new_path_count] != NULL) {
        new_path_count++;
    }

    update_path(new_paths, new_path_count);
}

/***************************************************************
****************************************************************
NB: ^^^^ FUNCTION TO REMOVE QUOTES FROM A STRING ^^^^
****************************************************************
****************************************************************/
char* remove_quotes(char* str) {
    size_t len = strlen(str);
    if (len > 1 && str[0] == '"' && str[len - 1] == '"') {
        str[len - 1] = '\0';
        str++;
    }
    return str;
}

/***************************************************************
****************************************************************
NB: ^^^^ FUNCTION TO PASS COMMAND STRINGS INTO  COMMAND TOKENS
        HANDLING ENVIRONMENT ENVIRONMENT VARIABLES,
        QUOTED STRINGS, AND REGULAR TOKENS ^^^^
****************************************************************
****************************************************************/
void tokenize_command(char* command, char* argv[], int* argc) {
    char* token = strtok(command, " \t");
    
    while (token != NULL) {
        // Check if the token starts with a $
        if (token[0] == '$') {
            // Remove the $ sign and look up the environment variable
            char *env_var_value = getenv(token + 1); // token + 1 skips the $

            if (env_var_value != NULL) {
                // If the environment variable exists, add its value to the argv list
                argv[(*argc)++] = strdup(env_var_value);
            } else {
                // If the environment variable doesn't exist, treat it as an empty string
                argv[(*argc)++] = strdup("");
            }
        } else if (token[0] == '"') {
            // Handle quoted strings (as already implemented)
            size_t buffer_size = strlen(token) + 1;
            char* quoted_string = (char*) malloc(buffer_size);
            strcpy(quoted_string, token);

            while (quoted_string[strlen(quoted_string) - 1] != '"' && (token = strtok(NULL, " \t")) != NULL) {
                buffer_size += strlen(token) + 1;
                quoted_string = realloc(quoted_string, buffer_size);
                
                strcat(quoted_string, " ");
                strcat(quoted_string, token);
            }

            if (quoted_string[strlen(quoted_string) - 1] == '"') {
                quoted_string = remove_quotes(quoted_string);
            }

            argv[(*argc)++] = quoted_string;
        } else {
            // Regular token (no $ and no quotes)
            argv[(*argc)++] = token;
        }

        token = strtok(NULL, " \t");
    }
}

/***************************************************************
****************************************************************
NB: ^^^^ FUNCTION TO HANDLE REDIRECTION ^^^^
****************************************************************
****************************************************************/
int handle_redirection(char *command) {
    char *output_file = NULL;
    char *redirection = strchr(command, '>');

    if (redirection != NULL) {
        if (redirection == command || (redirection == command + strspn(command, " \t"))) {
            char error_message[30] = "An error has occurred\n" ;
            write(STDERR_FILENO, error_message, strlen(error_message));
            return -1;
        }

        if (strchr(redirection + 1, '>') != NULL) {
            char error_message[30] = "An error has occurred\n" ;
            write(STDERR_FILENO, error_message, strlen(error_message));
            return -1;
        }

        *redirection = '\0';
        output_file = redirection + 1;

        while (*output_file == ' ' || *output_file == '\t') output_file++;
        char *output_end = output_file + strlen(output_file) - 1;
        while (output_end > output_file && (*output_end == ' ' || *output_end == '\t')) output_end--;
        *(output_end + 1) = '\0';

        if (output_file[0] == '\0') {
            char error_message[30] = "An error has occurred\n" ;
            write(STDERR_FILENO, error_message, strlen(error_message));
            return -1;
        }

        if (strtok(output_file, " \t") != NULL && strtok(NULL, " \t") != NULL) {
            char error_message[30] = "An error has occurred\n" ;
            write(STDERR_FILENO, error_message, strlen(error_message));
            return -1;
        }

        int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            char error_message[30] = "An error has occurred\n" ;
            write(STDERR_FILENO, error_message, strlen(error_message));
            return -1;
        }

        // Redirect stdout and stderr to the file
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        close(fd);

        return 1;
    }

    return 0;
}

/***************************************************************
****************************************************************
NB: ^^^^ FUNCTION TO HANDLE AND EXECUTE EACH COMMAND 
        INDIVIDUALLY ^^^^
****************************************************************
****************************************************************/
void execute_single_command(char *command) {
    if (strncmp(command, "cd ", 3) == 0) {
        handle_cd(command + 3);
        return;
    } else if (strncmp(command, "path", 4) == 0) {
        // Skip the "path" and leading spaces
        char *remaining_args = command + 4;
        while (*remaining_args == ' ') remaining_args++;

        if (*remaining_args == '\0') {
            // No arguments after 'path', clear the path
            char *empty_paths[] = {NULL};
            handle_path(empty_paths);  // Call handle_path with an empty list
        } else {
            // Handle the normal case where there are arguments
            char *path_args[128];
            int path_arg_count = 0;
            tokenize_command(remaining_args, path_args, &path_arg_count);
            path_args[path_arg_count] = NULL;
            handle_path(path_args);
        }
        return;
    } else if (strncmp(command, "exit", 4) == 0) {
        char *exit_args[128];
        int exit_arg_count = 0;
        tokenize_command(command + 4, exit_args, &exit_arg_count);

        if (exit_arg_count > 0) {
            char error_message[30] = "An error has occurred\n" ;
            write(STDERR_FILENO, error_message, strlen(error_message));
            return;
        }
        exit(0);
    }

    pid_t pid = fork();

    if (pid < 0) {
        char error_message[30] = "An error has occurred\n" ;
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    } else if (pid == 0) {
        if (handle_redirection(command) < 0) {
            exit(1);
        }

        char *argv[128];
        int argc = 0;
        tokenize_command(command, argv, &argc);
        argv[argc] = NULL;

        char *executable = find_executable(argv[0]);
        if (executable == NULL) {
            char error_message[30] = "An error has occurred\n" ;
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        }

        execv(executable, argv);
        char error_message[30] = "An error has occurred\n" ;
        write(STDERR_FILENO, error_message, strlen(error_message)); 
        free(executable);
        exit(1);
    } else {
        // Parent process should wait for child to finish
        int status;
        waitpid(pid, &status, 0);
    }
}

/***************************************************************
****************************************************************
NB: ^^^^ FUNCTION TO PARSE AND EXECUTE COMMANDS,
        INCLUDING PARALLEL COMMANDS ^^^^
****************************************************************
****************************************************************/
void execute_command(char *line, FILE *input_file) {
    if (line[0] == '\0' || line[0] == '#') {
        return;
    }

    char *carriage_return = strchr(line, '\r');
    if (carriage_return != NULL) {
        *carriage_return = '\0';
    }

    char *start = line;
    while (*start == ' ' || *start == '\t') start++;
    char *end = start + strlen(start) - 1;
    while (end > start && (*end == ' ' || *end == '\t')) end--;
    *(end + 1) = '\0';

    char *commands[128];
    int command_count = 0;

    char *command = strtok(start, "&");
    while (command != NULL) {
        commands[command_count++] = command;
        command = strtok(NULL, "&");
    }

    for (int i = 0; i < command_count; i++) {
        execute_single_command(commands[i]);
    }

    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        // Handle process termination if needed
    }
}

/***************************************************************
****************************************************************
NB: MAIN FUNCTION
****************************************************************
****************************************************************/
int main(int argc, char *argv[]) {
    paths = malloc(sizeof(char *));
    if (paths == NULL) {
        char error_message[30] = "An error has occurred\n" ;
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }
    paths[0] = strdup("/bin/");
    if (paths[0] == NULL) {
        char error_message[30] = "An error has occurred\n" ;
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }
    path_count = 1;

    FILE *input_file = stdin;
    if (argc > 1) {
        input_file = fopen(argv[1], "r");
        if (input_file == NULL) {
            char error_message[30] = "An error has occurred\n" ;
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        }

        // Check if the file is empty
        fseek(input_file, 0, SEEK_END);
        if (ftell(input_file) == 0) {
            char error_message[30] = "An error has occurred\n" ;
            write(STDERR_FILENO, error_message, strlen(error_message));
            fclose(input_file);
            exit(1);
        }
        fseek(input_file, 0, SEEK_SET);
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t nread;

    while (1) {
        if (input_file == stdin) {
            printf("witsshell> ");
            fflush(stdout);
        }

        nread = getline(&line, &len, input_file);
        if (nread == -1) {
            // End of file or error
            if (feof(input_file)) {
                break;  // End of file
            } else {
                char error_message[30] = "An error has occurred\n" ;
                write(STDERR_FILENO, error_message, strlen(error_message));
                free(line);
                for (int i = 0; i < path_count; i++) {
                    free(paths[i]);
                }
                free(paths);
                if (input_file != stdin) {
                    fclose(input_file);
                }
                exit(1);
            }
        }

        if (line[nread - 1] == '\n') {
            line[nread - 1] = '\0';
        }

        execute_command(line, input_file);
    }

    free(line);
    for (int i = 0; i < path_count; i++) {
        free(paths[i]);
    }
    free(paths);

    if (input_file != stdin) {
        fclose(input_file);
    }

    return 0;
}