/* COMP 530: Tar Heel SHell
 *
 * This file implements a table of builtin commands.
 */
// PID:730723179
// PID:730723162
#include "thsh.h"
#include <stdlib.h>
#include <string.h>

struct builtin {
  const char * cmd;
  int (*func)(char *args[MAX_ARGS], int stdin, int stdout);
};


static char old_path[MAX_INPUT];
static char cur_path[MAX_INPUT];
// static char usr_path[MAX_INPUT];

/* This function needs to be called once at start-up to initialize
 * the current path.  This should populate cur_path.
 *
 * Returns zero on success, -errno on failure.
 */
int init_cwd(void) {

  // Lab 2: Your code here
  memset(cur_path,0,sizeof(cur_path)); // Initialize cur_path
  char *current_dir = getcwd(NULL, 0); // Get the current working directory and copy it to a new varible current_dir
  if (!current_dir) {
      perror("getcwd");
      return -errno;
  }
  strcpy(cur_path,current_dir);  // copy current_dir to cur_path
  return 0;
}

// the function used for cd ..
int up_dir(char new_dir[MAX_INPUT]){
  int i=strlen(cur_path)-1;
  for(;cur_path[i]!='/';i--); // move the i from the bottom of cur_path to the first backslash(the upper dir)
  if(i<=0) return 1;  // there is no valid up_dir
  for(int j=0;j<i;j++){  // copy
    new_dir[j]=cur_path[j];
  }
  return 0;
}

/* Handle a cd command.  */
int handle_cd(char *args[MAX_INPUT], int stdin, int stdout) {

  // Note that you need to handle special arguments, including:
  // "-" switch to the last directory
  // "." switch to the current directory.  This should change the
  //     behavior of a subsequent "cd -"
  // ".." go up one directory
  //
  // Hint: chdir can handle "." and "..", but saving
  //       these results may not be the desired outcome...

  // XXX: Test for errors in the output if a cd fails

  // Lab 2: Your code here
  //

  // Remove the following two lines once implemented.  These
  // just suppress the compiler warning around an unused variable
  
  if(args[1]==NULL){
    dprintf(stdout, "Invalid directory.\n");
    return 0;
  }

  // initialize the old_path and copy the previous cur_path to old_path as current old_path
  memset(old_path,0,sizeof(old_path));
  strcpy(old_path,cur_path);

  // get current path by calling init_cwd
  if(init_cwd()!=0){
    dprintf(stdout, "Current path initialization error.\n");
    return 1;
  }

  // according to different cd command to create different new_dir
  char new_dir[MAX_INPUT];
  strcpy(new_dir,args[1]);
  if (new_dir==NULL || strcmp(new_dir, ".") == 0) {
    strcpy(new_dir,cur_path);
  } 
  else if (strcmp(new_dir, "-") == 0) {
    // Handle "-" to switch to the last directory
    if (old_path==NULL) {
      dprintf(stdout, "No previous directory available.\n");
      return 1;
    }
    strcpy(new_dir,old_path);
  }
  else if (strcmp(new_dir, "..") == 0){
    if(up_dir(new_dir)){
      dprintf(stdout, "No up directory available.\n");
      return 1;
    }

  }

  // change dir to new_dir
  if (chdir(new_dir) != 0) {
    dprintf(stdout, "Failed to run command - error -1\n");
    return 1;
  }

  return 0;
}

/* Handle an exit command. */
int handle_exit(char *args[MAX_ARGS], int stdin, int stdout) {
  exit(0);
  return 0; // Does not actually return
}

int handle_goheels(char *args[MAX_ARGS], int stdin, int stdout){
  char goheels[7][61] = {
      "   ______            ____  ____               __          ",
      " .' ___  |          |_   ||   _|             [  |         ",
      "/ .'   \\_|   .--.     | |__| |  .---.  .---.  | |  .--.   ",
      "| |   ____ / .'`\\ \\   |  __  | / /__\\\\/ /__\\\\ | | ( (`\\]  ",
      "\\ `.___]  || \\__. |  _| |  | |_| \\__.,| \\__., | |  `\\.'  ",
      " `._____.'  '.__.'  |____||____|'.__.' '.__.'[___][\\__) ) ",
      "                                                          "
  };
  char nc[17][53] = {
    "          :=*%@*.    .::-----::..     *@#+:       ",
    "         *@@@@@@@%%@@@@@@@@@@@@@@@%#*#@@@@@%      ",
    "          @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@:      ",
    "       -#@@@@@@@@@@@@@%%%%%%%%%@@@@@@@@@@@@@#=    ",
    "     -%@@@@@@@@@@@@@@@*.          :%@@@@@@@@@@@=  ",
    "   .%@@@@@@%@@@@@@@@@@@@+          @@@@@@%@@@@%-  ",
    "  :@@@@@@#: *@@@@@@@@@@@@@-       -@@@@@% .*%-    ",
    "  %@@@@@+   =@@@@@*:#@@@@@@%-     +@@@@@*         ",
    " :@@@@@@    =@@@@@#  :#@@@@@@%:   #@@@@@*         ",
    " :@@@@@@    =@@@@@*    -%@@@@@@*. *@@@@@*         ",
    "  #@@@@@#   =@@@@@+      =@@@@@@@+#@@@@@*   .     ",
    "  .%@@@@@@= #@@@@@-       .*@@@@@@@@@@@@@ -%@+.   ",
    "    *@@@@@@@@@@@@@          .#@@@@@@@@@@@@@@@@@+  ",
    "     .*@@@@@@@@@@@=-:.        -@@@@@@@@@@@@@@@#:  ",
    "       .+%@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@+:    ",
    "         +@@@@@*-=+##%@@@@@@@@@%%#++*@@@@@@*      ",
    "           .-+=                      :**=:        "
  };
  dprintf(stdout,"\n");
  for (int i = 0; i < 7; i++) {
    dprintf(stdout,"%s\n", goheels[i]);
  }
  for (int i = 0; i < 17; i++) {
    dprintf(stdout,"%s\n", nc[i]);
  }
  dprintf(stdout,"\n");
  return 0;
}

static struct builtin builtins[] = {{"cd", handle_cd},
                                    {"exit", handle_exit},
                                    {"goheels", handle_goheels},
                                    {NULL, NULL}};

/* This function checks if the command (args[0]) is a built-in.
 * If so, call the appropriate handler, and return 1.
 * If not, return 0.
 *
 * stdin and stdout are the file handles for standard in and standard out,
 * respectively. These may or may not be used by individual builtin commands.
 *
 * Places the return value of the command in *retval.
 *
 * stdin and stdout should not be closed by this command.
 *
 * In the case of "exit", this function will not return.
 */
int handle_builtin(char *args[MAX_ARGS], int stdin, int stdout, int *retval) {
  int rv = 0;
  // Lab 0: Your Code Here
  // Comment this line once implemented.  This just suppresses
  // the unused variable warning from the compiler.
  // (void) builtins;
  for(int i=0;builtins[i].cmd!=NULL;i++){
    if(strcmp(args[0],builtins[i].cmd)==0){
        *retval=builtins[i].func(args,stdin,stdout);
        rv=1;
    }
  }
  return rv;
}

/* This function initially prints a default prompt of:
 * thsh>
 *
 * In Lab 2, Exercise 3, you will add the current working
 * directory to the prompt.  As in, if you are in "/home/foo"
 * the prompt should be:
 * [/home/foo] thsh>
 *
 * Returns the number of bytes written
 */
int print_prompt(void) {
  int ret = 0;
  // Print the prompt
  // file descriptor 1 -> writing to stdout
  // print the whole prompt string (write number of
  // bytes/chars equal to the length of prompt)
  //
  const char *prompt = "thsh> ";

  // Lab 2: Your code here
  char *current_dir = getcwd(NULL, 0); // Get the current working directory
  // whole_prompt='[' + 'current_dir' + ']' + ' '
  char whole_prompt[100];
  memset(whole_prompt,0,100);
  whole_prompt[0]='[';
  strcpy(whole_prompt+1,current_dir);
  whole_prompt[strlen(whole_prompt)]=']';
  whole_prompt[strlen(whole_prompt)]=' ';
  
  // cat prompt after whole_prompt and return them
  strcat(whole_prompt,prompt);
  ret = write(1, whole_prompt, strlen(whole_prompt));
  return ret;
}
