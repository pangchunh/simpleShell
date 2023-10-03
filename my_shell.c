#include  <stdio.h>
#include  <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>


#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64

/* Splits the string by space and returns the array of tokens
*
*/
char **tokenize(char *line)
{
  char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
  char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
  int i, tokenIndex = 0, tokenNo = 0;

  for(i =0; i < strlen(line); i++){

    char readChar = line[i];

    if (readChar == ' ' || readChar == '\n' || readChar == '\t'){
      token[tokenIndex] = '\0';
      if (tokenIndex != 0){
	tokens[tokenNo] = (char*)malloc(MAX_TOKEN_SIZE*sizeof(char));
	strcpy(tokens[tokenNo++], token);
	tokenIndex = 0; 
      }
    } else {
      token[tokenIndex++] = readChar;
    }
  }
 
  free(token);
  tokens[tokenNo] = NULL ;
  return tokens;
}

/**
 * Extract the options/arguments for the command supplied.
 * @param tokens
 * @return an array of argument with NULL at the end
 */
char **getOptions(char **tokens){
    char **options = (char **) malloc(sizeof tokens);
    int arr_length = sizeof (tokens) / sizeof (tokens[0]);
    for (int i = 0; i < arr_length - 1; i++) {
        options[i] = tokens[i + 1];
    }
    options[arr_length] = NULL;
    return options;
}

int changeDirectory(char **tokens){
    char path[MAX_TOKEN_SIZE];
    strcpy(path, tokens[1]);
    if (chdir(path) != 0){
        perror("Shell: Incorrect command");
        return -1;
    }
    return 0;
}

/**
 * Create a child process to execute linux executables in the /bin folder.
 * @param tokens of the command entered.
 * @return 0 if execution is successful, otherwise -1
 */
int executeBasic(char **tokens){
    char command[MAX_TOKEN_SIZE];
    snprintf(command, sizeof command, "/bin/%s", tokens[0]);
    int rc = fork();
    if (rc < 0){
        perror("Fork failed");
        return -1;
    } else if (rc == 0){
        execv(command, tokens);
        perror("Exec failed");
    } else{
        wait(NULL);
    }
}

int main(int argc, char* argv[]) {
	char  line[MAX_INPUT_SIZE];            
	char  **tokens;              
	int i;

	while(1) {			
		/* BEGIN: TAKING INPUT */
		bzero(line, sizeof(line));
        char cwd[MAX_INPUT_SIZE];
        getcwd(cwd, sizeof(cwd));
		fprintf(stdout, "%s $ ", cwd);
		scanf("%[^\n]", line);
		getchar();

		printf("Command entered: %s (remove this debug output later)\n", line);
		/* END: TAKING INPUT */

		line[strlen(line)] = '\n'; //terminate with new line
		tokens = tokenize(line);

        /* Main execution logic */
        if (strcmp(tokens[0], "cd") == 0){
            changeDirectory(tokens);
        } else{
            executeBasic(tokens);
        }

        // Freeing the allocated memory
		for(i=0;tokens[i]!=NULL;i++){
			free(tokens[i]);
		}
		free(tokens);

	}
	return 0;
}


