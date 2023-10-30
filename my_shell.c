#include  <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>


#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64
#define MAX_NUM_COMMAND 64

pid_t foreground_pid;
pid_t parallell_pids[64];
int volatile cltr_c = 0;

/* Splits the string by space and returns the array of tokens
*
*/
char **tokenize(char *line) {
    char **tokens = (char **) malloc(MAX_NUM_TOKENS * sizeof(char *));
    char *token = (char *) malloc(MAX_TOKEN_SIZE * sizeof(char));
    int i, tokenIndex = 0, tokenNo = 0;

    for (i = 0; i < strlen(line); i++) {

        char readChar = line[i];

        if (readChar == ' ' || readChar == '\n' || readChar == '\t') {
            token[tokenIndex] = '\0';
            if (tokenIndex != 0) {
                tokens[tokenNo] = (char *) malloc(MAX_TOKEN_SIZE * sizeof(char));
                strcpy(tokens[tokenNo++], token);
                tokenIndex = 0;
            }
        } else {
            token[tokenIndex++] = readChar;
        }
    }
    free(token);
    tokens[tokenNo] = NULL;
    return tokens;
}

int changeDirectory(char **tokens) {
    char path[MAX_TOKEN_SIZE];

    int tokens_length = 0;
    while (tokens[tokens_length] != NULL) {
        tokens_length++;
    }
    if (tokens_length < 2) {
        printf("Shell: Incorrect command\n");
        return -1;
    }
    strcpy(path, tokens[1]);
    if (chdir(path) != 0) {
        perror("Shell: Incorrect command");
        return -1;
    }
    return 0;
}

int executeBackGroundProcess(char **tokens) {
    char command[MAX_TOKEN_SIZE];
    snprintf(command, sizeof command, "/bin/%s", tokens[0]);
    int rc = fork();
    if (rc < 0) {
        perror("Fork failed");
        return -1;
    } else if (rc == 0) {
        close(STDIN_FILENO);
        execv(command, tokens);
        perror("Error execv");
        exit(1);
    } else {
        setpgid(rc, rc);
        printf("Child process running in background with PID %d\n", rc);
        return rc;
    }
}

/**
 * Create a child process to execute linux executables in the /bin folder.
 * @param tokens of the command entered.
 * @return 0 if execution is successful, otherwise -1
 */
int executeBasic(char **tokens, bool parallell, int index) {
    if (tokens[index] == NULL) {
        printf("token[%d] is null, wth\n", index);
        return 0;
    }
    char command[MAX_TOKEN_SIZE];

    snprintf(command, sizeof command, "/bin/%s", tokens[index]);

    int rc = fork();
    if (rc < 0) {
        perror("Fork failed");
        return -1;
    } else if (rc == 0) {
        setpgid(0, 0); //create a new process group
        execvp(tokens[index], (char **) &tokens[index]);
        perror("Exec failed");
    } else if(parallell == true){
        return rc;
    } else {
        int status;
        foreground_pid = rc;
        waitpid(rc, &status, 0);
    }
    return rc;
}

void killAllBackgroundProcess(pid_t background_pids[], int num_background_pids) {
    for (int i = 0; i < num_background_pids; i++) {
        kill(background_pids[i], SIGTERM);
        int status;
        waitpid(background_pids[i], &status, 0);
    }

}

/**
 * collect the terminated background process and reap it.
 * @param background_pids
 * @param num_background_pids
 */
void reapTerminatedBackgroundProcess(pid_t background_pids[], int *num_background_pids) {
    int num_active_background_pids = 0;

    printf("Number of background process %d\n", *num_background_pids);

    for (int i = 0; i < *num_background_pids; i++) {
        pid_t bg_pid = background_pids[i];
        int status;
        int result = waitpid(bg_pid, &status, WNOHANG);
        background_pids = 0;
        if (result > 0) {
            printf("Shell: Background process finished.\n");
        } else {
            background_pids[num_active_background_pids] = bg_pid;
            num_active_background_pids++;
        }
    }
    *num_background_pids = num_active_background_pids;
}

/**
 * Define signal handler to handle ctrl+C operation
 */
void signalHandler() {
    if (foreground_pid > 0 || parallell_pids[0] > 0) {
        cltr_c = 1;
        printf("\nCaught SIGINT signal, terminating foreground process\n");
        kill(-foreground_pid, SIGINT);
        foreground_pid = 0;
        for(int i = 0; parallell_pids[i] != 0; i++){
            kill(-parallell_pids[i], SIGINT);
            int status;
            waitpid(parallell_pids[i], &status, 0);
            parallell_pids[i] = 0;
        }
    }
}

/**
 * Determine distinct commands and put the starting position to the command queue.
 * @param tokens pointer to array of strings
 * @param command_queue pointer to the command queue
 * @return number of command
 */
int queue_tokens(char **tokens, int *command_queue) {
    int commands_count = 1;
    command_queue[0] = 0;
    int i;
    for (i = 0; tokens[i] != NULL; i++) {
        if (strcmp(tokens[i], "&&") == 0 || strcmp(tokens[i], "&&&") == 0) {
            tokens[i] = NULL;
            command_queue[commands_count] = i + 1;
            commands_count++;
        }
    }
    return commands_count;
}

bool containsToken(char **tokens, char *token) {
    for (int i = 0; tokens[i] != NULL; i++) {
        if (strcmp(tokens[i], token) == 0) {
            return true;
        }
    }
    return false;
}


int main(int argc, char *argv[]) {
    char line[MAX_INPUT_SIZE];
    char **tokens;
    int i;
    pid_t background_pids[MAX_TOKEN_SIZE];
    int num_background_pids = 0;
    signal(SIGINT, signalHandler);

    while (1) {
        /* BEGIN: TAKING INPUT */
        bzero(line, sizeof(line));
        char cwd[MAX_INPUT_SIZE];
        getcwd(cwd, sizeof(cwd));
        fprintf(stdout, "%s $ ", cwd);
        scanf("%[^\n]", line);
        getchar();
        /* END: TAKING INPUT */

        line[strlen(line)] = '\n'; //terminate with new line
        tokens = tokenize(line);

        int tokens_length = 0;
        cltr_c = 0;
        while (tokens[tokens_length] != NULL) {
            tokens_length++;
        }
        if (tokens_length == 0){
            continue;
        }

        /* Reap terminated background process */
        reapTerminatedBackgroundProcess(background_pids, &num_background_pids);

        /* Main execution logic */
        if (strcmp(tokens[0], "cd") == 0) {
            changeDirectory(tokens);
        } else if (strcmp(tokens[tokens_length - 1], "&") == 0) {
            tokens[tokens_length - 1] = NULL;
            int background_pid = executeBackGroundProcess(tokens);
            background_pids[num_background_pids] = background_pid;
            num_background_pids++;
        } else if (strcmp(tokens[0], "exit") == 0) {
            printf("Thank you for using my shell\n");
            killAllBackgroundProcess(background_pids, num_background_pids);
            for (i = 0; tokens[i] != NULL; i++) {
                free(tokens[i]);
            }
            free(tokens);
            exit(0);
        } else if (containsToken(tokens, "&&&")) {
            int command_queue[MAX_TOKEN_SIZE];
            int command_counts = queue_tokens(tokens, command_queue);
            for (int i = 0; i < command_counts; i++) {
                parallell_pids[i] = executeBasic(tokens, true, command_queue[i]);
            }
            for(int i = 0; i < command_counts; i++){
                int status;
                waitpid(parallell_pids[i], &status, 0);
            }
        } else if (containsToken(tokens, "&&")) {
            int command_queue[MAX_TOKEN_SIZE];
            int command_counts = queue_tokens(tokens, command_queue);
            printf("Command count %d\n", command_counts);
//            SERIALIZATION
            for (int i = 0; i < command_counts; i++) {
                foreground_pid = executeBasic(tokens, 0, command_queue[i]);
                if (cltr_c) {
                    break;
                }
            }
        } else if(tokens_length > 0){
            executeBasic(tokens, 0, 0);
        }

        // Freeing the allocated memory
        for (i = 0; tokens[i] != NULL; i++) {
            free(tokens[i]);
        }
        free(tokens);

    }
    return 0;
}



