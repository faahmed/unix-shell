#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "dynarray.h"

#define RC "/.ishrc"
#define LEN 1023
#define MAX_TOKENS 511
#define MAX_TOKEN_SIZE 800

int execruncom();
int prompt();

//executes runcom and then waits for input from stdin
int main(void) {
  execruncom();
  while (1) {
    prompt();
  }
  return 0;
}

void addtoken(DynArray_T* current_token, DynArray_T* tokens) {

    int len_current_token = DynArray_getLength(*current_token);

    char **token_ptr;
    token_ptr = (char**) malloc(sizeof(int));

    char *token;
    token = (char *)malloc(sizeof(char)*(len_current_token+1));

    *token_ptr = token;

    for (int i=0; i<len_current_token; i++) {

      token[i] = *(int*)DynArray_get(*current_token, i);

    }

    token[len_current_token] = '\0';

    DynArray_add(*tokens, token_ptr);
    *current_token = DynArray_new(0);

}

char** parse(DynArray_T* tokens) {

  int tokens_length = DynArray_getLength(*tokens);
  int input_redirects = 0;
  int output_redirects = 0;

  char **argv;
  argv = malloc(MAX_TOKENS * sizeof(char*));
  for (int i=0; i < MAX_TOKENS; i++) {
    argv[i] = malloc(sizeof(char)*MAX_TOKEN_SIZE);
  }

  for (int i=0; i < tokens_length; i++) {

    char* token = *(char**)DynArray_get(*tokens, i);
    char* prev = NULL;
    char* next = NULL;

    if (i != 0) {
      prev = *(char**)DynArray_get(*tokens, i-1);
    }
    if (i != tokens_length-1) {
      next = *(char**)DynArray_get(*tokens, i+1);
    }
    
    if (strcmp(token, "<") == 0) {
      if (prev==NULL) {
        perror("Invalid: Missing command name");
      }
      if (next==NULL) {
        perror("Invalid: Standard input redirection without file name");
        continue;
      }
      input_redirects++;
      if (input_redirects > 1) {
        perror("Invalid: Multiple redirection of standard input");
      }
    } else if (strcmp(token, ">") == 0) {
      if (next==NULL) {
        perror("Invalid: Standard output redirection without file name");
        continue;
      }
      output_redirects++;
      if (output_redirects > 1) {
        perror("Invalid: Multiple redirection of standard output");
      }
    } 
    argv[i] = *(char**)DynArray_get(*tokens, i);
  }
  argv[tokens_length] = NULL;
  return argv;
}

int execute(char** argv) {

    if (strcmp(argv[0], "setenv") == 0) {
      if (argv[2] == NULL) {
        setenv(argv[1],"",1);
      } else {
        setenv(argv[1], argv[2], 1);
      }
    } else if (strcmp(argv[0], "unsetenv") == 0) {
        unsetenv(argv[1]);
    } else if (strcmp(argv[0], "cd") == 0) {
      if (argv[1] == NULL) {
        chdir(getenv("HOME"));
      } else {
        chdir(argv[1]);
      }
    } else if (strcmp(argv[0], "exit") == 0) {
      exit(EXIT_SUCCESS);
    } else {

      pid_t child_pid;
      int child_status;
      fflush(NULL);

      child_pid = fork();

      if (child_pid == 0) {
        execvp(argv[0], argv);
        perror("Unknown command");
        exit(0);
      } else {

          pid_t tpid;

          do {
            tpid = wait(&child_status);
          } while (tpid != child_pid);

          return child_status;
      }

    }
    free(argv);
    return 1;
}

int lex(char* str, DynArray_T* tokens ,int print) {

  DynArray_T current_token   = DynArray_new(0);
  int        unmatched_quote = 0;

  if (print) printf("%% %s", str);
  
  for (int i=0; i < LEN; i++) {

    switch(str[i]) {
      
      case '\0': 

        if (unmatched_quote) {
          perror("ERROR - unmatched quote");
          for (int i=0; i < DynArray_getLength(*tokens); i++) {
            free(*(char**)DynArray_get(*tokens, i));
            free((char**)DynArray_get(*tokens, i));
          }
        return(0);
        } 
        break;

      case '<':
      case '>':
        if (DynArray_getLength(current_token) > 0) {
          addtoken(&current_token, tokens);
        }
        DynArray_add(current_token, &str[i]);
        addtoken(&current_token, tokens);
        break;

      case ' ':
      case '\t':
      case '\n':
      case '\v':
      case '\f':
      case '\r':
        if (unmatched_quote) {
          DynArray_add(current_token, &str[i]);
        } else {
          if (DynArray_getLength(current_token) > 0) {
            addtoken(&current_token, tokens);
          }
        }
        break;

      case '"':
        unmatched_quote ^= 1;
        break;

      default:
        DynArray_add(current_token, &str[i]);
    }

  }
  
  char** argv = parse(tokens);
  execute(argv);
  return 1;
}

//executes commands in the runcom file
int execruncom() {

  FILE *runcom = fopen(strcat(getenv("HOME"), RC), "r");
  
  if (runcom == NULL) {
    perror("Error opening runcom file");
    return(-1);
  }

  while (1) {

    char str[LEN+1];
    memset(str, '\0', sizeof(char) * (LEN+1));
    
    if(fgets(str, LEN, runcom) != NULL) {

      DynArray_T tokens = DynArray_new(0);
      if (!lex(str, &tokens , 1)) {
        perror("Error reading lines from runcom file");
      }
      for (int i=0; i < DynArray_getLength(tokens); i++) {
        free(*(char**)DynArray_get(tokens, i));
        free((char**)DynArray_get(tokens, i));
      }

    } else {

      break;

    }
  }

  fclose(runcom);
  return 0;

}

int prompt() {
  printf("%% ");

    char str[LEN+1];

    memset(str, '\0', sizeof(char) * (LEN+1));

    DynArray_T tokens = DynArray_new(0);

    fgets(str,LEN+1,stdin);
    lex(str, &tokens ,0);

    for (int i=0; i < DynArray_getLength(tokens); i++) {
      free(*(char**)DynArray_get(tokens, i));
      free((char**)DynArray_get(tokens, i));
    }
    return 0;
}
