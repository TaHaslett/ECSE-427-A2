#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/wait.h>
#include <dirent.h>
#include <stdbool.h>
#include "shellmemory.h"
#include "shell.h"
#include "scripts.h"
#include "scheduler.h"

int MAX_ARGS_SIZE = 100;

int badcommand() {
    printf("Unknown Command\n");
    return 1;
}

// For source command only
int badcommandFileDoesNotExist() {
    printf("Bad command: File not found\n");
    return 3;
}

int badcommandFileDoesNotFit() {
    printf("Bad command: File does not fit in script memory\n");
    return 4;
}

int help();
int quit();
int set(char *var, char *value);
int print(char *var);
int source(char *script);
int echo(char *var);
int my_ls();
int my_mkdir(char *dirname);
int my_touch(char *filename);
int my_cd(char *dirname);
int run(char *command_args[]);
int exec(char *command_args[], int args_size);

// Interpret commands and their arguments
int interpreter(char *command_args[], int args_size) {
    int i;

    if (args_size < 1 || args_size > MAX_ARGS_SIZE) {
        return badcommand();
    }

    for (i = 0; i < args_size; i++) {   // terminate args at newlines
        command_args[i][strcspn(command_args[i], "\r\n")] = 0;
    }

    if (strcmp(command_args[0], "help") == 0) {
        //help
        if (args_size != 1)
            return badcommand();
        return help();

    } else if (strcmp(command_args[0], "quit") == 0) {
        //quit
        if (args_size != 1)
            return badcommand();
        return quit();

    } else if (strcmp(command_args[0], "set") == 0) {
        //set
        if (args_size != 3)
            return badcommand();
        return set(command_args[1], command_args[2]);

    } else if (strcmp(command_args[0], "print") == 0) {
        if (args_size != 2)
            return badcommand();
        return print(command_args[1]);

    } else if (strcmp(command_args[0], "source") == 0) {
        if (args_size != 2)
            return badcommand();
        return source(command_args[1]);

    } else if (strcmp(command_args[0], "echo") == 0) {
        if (args_size != 2)
	        return badcommand();
        return echo(command_args[1]);

    } else if (strcmp(command_args[0], "my_ls") == 0) {
    	if (args_size != 1)
	        return badcommand();
	    return my_ls();
    
    } else if (strcmp(command_args[0], "my_mkdir") == 0) {
	if (args_size != 2)
	        return badcommand();
	    return my_mkdir(command_args[1]);
    
    } else if (strcmp(command_args[0], "my_touch") == 0) {
        if (args_size != 2)
	        return badcommand();
	    return my_touch(command_args[1]);
    
    } else if (strcmp(command_args[0], "my_cd") == 0) {
        if (args_size != 2)
            return badcommand();
        return my_cd(command_args[1]);
    
    } else if (strcmp(command_args[0], "run") == 0) {
        if (args_size < 2)
            return badcommand();
        // prep command list
        // ensure that adding the NULL will not overflow the array
        if (args_size >= MAX_ARGS_SIZE) 
            return badcommand(); 
        // adds NULL to the end of the list of strings
        command_args[args_size] = NULL;
        // removes the first element which is the run command
        command_args++; 
        
        return run(command_args);

    } else if (strcmp(command_args[0], "mkdir") == 0) {
        if (args_size != 2)
            return badcommand();
        return my_mkdir(command_args[1]);

    } else if (strcmp(command_args[0], "touch") == 0) {
        if (args_size != 2)
            return badcommand();
        return my_touch(command_args[1]);

    } else if (strcmp(command_args[0], "cd") == 0) {
        if (args_size != 2)
            return badcommand();
        return my_cd(command_args[1]);

    } else if (strcmp(command_args[0], "exec") == 0) {
        if (args_size < 3 || args_size > 7) // exec + at least 1 file + policy, but no more than 3 files + policy + BackGroundMode + MultiThreadMode
            return badcommand();

        // removes the first element which is the exec command
        command_args++; 
        return exec(command_args, args_size - 1);

    } else
        return badcommand();
}

int help() {

    // note the literal tab characters here for alignment
    char help_string[] = "COMMAND			DESCRIPTION\n \
help			Displays all the commands\n \
quit			Exits / terminates the shell with “Bye!”\n \
set VAR STRING		Assigns a value to shell memory\n \
print VAR		Displays the STRING assigned to VAR\n \
source SCRIPT.TXT	Executes the file SCRIPT.TXT\n ";
    printf("%s\n", help_string);
    return 0;
}

int quit() {
    printf("Bye!\n");
    exit(0);
}

int set(char *var, char *value) {
    // Challenge: allow setting VAR to the rest of the input line,
    // possibly including spaces.

    // Hint: Since "value" might contain multiple tokens, you'll need to loop
    // through them, concatenate each token to the buffer, and handle spacing
    // appropriately. Investigate how `strcat` works and how you can use it
    // effectively here.

    mem_set_value(var, value);
    return 0;
}

int print(char *var) {
    printf("%s\n", mem_get_value(var));
    return 0;
}

Script *open_script(char *script, int start_idx) {
    FILE *p = fopen(script, "rt");
    if (p == NULL) {
        badcommandFileDoesNotExist();
        return NULL;
    }
    int i = start_idx;
    if (i >= MAX_LINES) {
        badcommandFileDoesNotFit();
        fclose(p);
        return NULL;
    }
    int lines = 0;
    while (fgets(script_memory[i], MAX_LINE_SIZE - 1, p) != NULL) {
        i++;
        lines++;
        if (i >= MAX_LINES) {
            badcommandFileDoesNotFit();
            fclose(p);
            return NULL;
        }
    }
    fclose(p);
    Script *s = create_script(start_idx, lines);
    return s;
}

int source(char *script) {
    int errCode = 0;
    Script *s = open_script(script, 0);
    if (s == NULL) {
        free(s);
        return 1;
    }
    errCode = scheduler(FCFS, s, NULL, NULL, NULL);
    return errCode;
}

int echo(char *var) {
    if (var[0] == '$') {
        char *val = mem_get_value(var + 1);
        if (strcmp(val, "Variable does not exist") == 0) {
            val = "";
        }
	    printf("%s\n", val);
    } else {
	    printf("%s\n", var);
    }
    return 0;
}

int my_ls() {
    struct dirent **dir_entry_list;
    int n = scandir(".", &dir_entry_list, NULL, alphasort);

    if (n < 0) {
        perror("my_ls couldn't scan the directory");
        return 1;
    }
    for (int i = 0; i < n; i++) {
        printf("%s\n", dir_entry_list[i]->d_name);
        free(dir_entry_list[i]);
    }
    free(dir_entry_list);
    return 0;
}

int is_single_alpha_numeric_token(char *str) {
    if (str == NULL || *str == '\0') {
        return 0;
    }

    for (int i = 0; str[i] != '\0'; i++) {
        if (!isalnum(str[i]) || str[i] == ' ') {
            return 0;
        }
    }
    return 1;
}

int my_mkdir(char *dirname) {
    int err = 0;
    if (dirname[0] == '$') {
	    char *val = mem_get_value(dirname + 1);
        if (strcmp(val, "Variable does not exist") == 0) 
            err = 1;
        dirname = val;
    } 
    
    if (!is_single_alpha_numeric_token(dirname) || err) {
        printf("Bad command: my_mkdir\n");
        return 1;
    }

    // Creates directory with the proper permissions
    mode_t mode = 
        S_IRUSR | S_IWUSR | S_IXUSR |   // usr:     rwx
        S_IRGRP | S_IXGRP |             // group:   r-x
        S_IROTH | S_IXOTH;              // others:  r-x
    if (mkdir(dirname, mode)) {
        perror("Something went wrong in my_mkdir");
        return 1;
    }
    return 0;
}

int my_touch(char *filename) {
    // Creates file with the proper permissions
    mode_t mode = 
        S_IRUSR | S_IWUSR |             // usr:     rw-
        S_IRGRP | S_IWGRP |             // group:   rw-
        S_IROTH;                        // others:  r--
    int file_descriptor = open(filename, O_WRONLY | O_CREAT | O_TRUNC, mode);
    if (file_descriptor == -1) 
        return -1;
    close(file_descriptor);
    return 0;
}

int my_cd(char *dirname) {
    if (chdir(dirname) != 0) {
        printf("Bad command: my_cd\n");
        return 1;
    }
    return 0;
}

int run(char *command_args[]) {
    fflush(stdout); // make sure parent output is printed
    pid_t pid = fork();
    if (pid < 0) {
        return 1;
    } else if (pid == 0) {
        execvp(command_args[0], command_args);
        exit(1);
    } else {
        int status;
        wait(&status);
        return 0;
    }
}

Policy parse_policy(char *policy_str) {
    if (strcmp(policy_str, "FCFS") == 0) {
        return FCFS;
    } else if (strcmp(policy_str, "SJF") == 0) {
        return SJF;
    } else if (strcmp(policy_str, "RR") == 0) {
        return RR;
    } else if (strcmp(policy_str, "RR30") == 0) {
        return RR30;
    } else if (strcmp(policy_str, "AGING") == 0) {
        return AGING;
    } else {
        printf("Bad command: exec command requires a valid scheduling policy\n");
        exit(1);
    }
}

Script *create_batch_script(int start_idx) {
    char line[MAX_LINE_SIZE];
    int lines = 0;
    int i = start_idx;
    while (fgets(line, MAX_LINE_SIZE, stdin) != NULL) {
        if (i >= MAX_LINES) {
            printf("Batch script does not fit in script memory\n");
            return NULL;
        }
        strcpy(script_memory[i], line);
        i++;
        lines++;
    }

    return create_script(start_idx, lines);
}

int exec(char *command_args[], int args_size) {
    static int memory_offset = 0; // static variable to keep track of the next available memory index for scripts
    int memory_start_idx = memory_offset;

    bool background_mode = false;
    bool multi_thread_mode = false;
    if (strcmp(command_args[args_size - 1], "MT") == 0) {
        multi_thread_mode = true;
        args_size--; // remove the MT argument from consideration
    }
    if (strcmp(command_args[args_size - 1], "#") == 0) {
        background_mode = true;
        args_size--; // remove the BG argument from consideration
    }
    Policy policy = parse_policy(command_args[args_size - 1]);
    args_size--; // remove the policy argument from consideration
    
    Script *scripts[3] = {NULL, NULL, NULL};
    int start_idx = memory_offset;
    for (int i = 0; i < args_size; i++) {
        char *file = command_args[i];
        // ensure that files do not have duplicate names
        for (int j = 0; j < i; j++) {
            if (strcmp(command_args[j], file) == 0) {
                printf("Bad command: duplicate script names\n");
                return 1;
            }
        }
        Script *s = open_script(file, start_idx);
        if (s == NULL) {
            for (int j = 0; j < i; j++) {
                free(scripts[j]);
            }
            return 1;
        }
        scripts[i] = s;
        memory_offset += s->length;
        start_idx += s->length;
    }
    Script *batch_script = NULL;
    if (background_mode && !scheduler_running) {
        batch_script = create_batch_script(start_idx);
        if (batch_script == NULL) {
            for (int j = 0; j < args_size; j++) {
                if (scripts[j] != NULL) {
                    free(scripts[j]);
                }
            }
            return 1;
        }
        memory_offset += batch_script->length;
    }
    int errCode = scheduler(policy, scripts[0], scripts[1], scripts[2], batch_script);
    return errCode;
}