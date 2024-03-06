/* COMP 530: Tar Heel SHell
 *
 * This file implements functions related to launching
 * jobs and job control.
 */
// PID:730723179
// PID:730723162

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "thsh.h"

#define PATH_N 1000
#define PATH_L 1000

static char ** path_table;

/* Initialize the table of PATH prefixes.
 *
 * Split the result on the parenteses, and
 * remove any trailing '/' characters.
 * The last entry should be a NULL character.
 *
 * For instance, if one's PATH environment variable is:
 *  /bin:/sbin///
 *
 * Then path_table should be:
 *  path_table[0] = "/bin"
 *  path_table[1] = "/sbin"
 *  path_table[2] = '\0'
 *
 * Hint: take a look at getenv().  If you use getenv, do NOT
 *       modify the resulting string directly, but use
 *       malloc() or another function to allocate space and copy.
 *
 * Returns 0 on success, -errno on failure.
 */
int init_path(void) {
  /* Lab 1: Your code here */
  // initialize path_table

  path_table = (char **)malloc(PATH_N * sizeof(char *));
  if (path_table == NULL) {
      return -errno;
  }

  // read paths from getenv("PATH") and copy them to pathtemp
  char *pathtemp = (char*)malloc(PATH_N*PATH_L*sizeof(char));
  char *path = getenv("PATH");
  if(path == NULL){
    // path_table[0]=". "; // it's for challenge 
    return 0;
  }
  for(int i=0;i<strlen(path);i++){
    char x=path[i];
    pathtemp[i]=x;
  }
  
  //use flag to avoid empty path
  int flag=0;
  for(int i=0;i<=strlen(pathtemp);i++){
    if((pathtemp[i]>='a'&&pathtemp[i]<='z')||(pathtemp[i]>='A'&&pathtemp[i]<='Z')||pathtemp[i]=='.'){
      flag=1;
      break;
    }
  }
  if(!flag) return 0;

  // create temp as temp spaces to contain letters
  int j=0,k=0;
  char temp[PATH_N][PATH_L];
  // read paths from pathtemp and add them to path_table
  for(int i=0;i<=strlen(pathtemp);i++){
    if(pathtemp[i]==':'&& pathtemp[i-1]!=':'){
      if(temp[j][k-1]=='/'){
        temp[j][k-1]='\0';
      }
      path_table[j]=temp[j];
      j++;
      k=0;
    }
    else if(pathtemp[i]=='\0'){
      if(temp[j][k-1]=='/'){
        temp[j][k-1]='\0';
      }
      path_table[j]=temp[j];
      break;
    }
    else if(pathtemp[i]=='/'&&pathtemp[i-1]=='/'){
      continue;
    }
    else if(pathtemp[i]==':'&&pathtemp[i-1]==':'){
      // challenge part
      // path_table[j]=". ";
      // j++;
      continue;
    }
    else{
      temp[j][k]=pathtemp[i];
      k++;
    }
  }

  free(pathtemp);
  return 0;
}

/* Debug helper function that just prints
 * the path table out.
 */
void print_path_table() {
  if (path_table == NULL) {
    printf("XXXXXXX Path Table Not Initialized XXXXX\n");
    return;
  }

  printf("===== Begin Path Table =====\n");
  for (int i = 0; path_table[i]; i++) {
    printf("Prefix %2d: [%s]\n", i, path_table[i]);
  }
  printf("===== End Path Table =====\n");
}

static int job_counter = 0;

struct kiddo {
  int pid;
  struct kiddo *next; // Linked list of sibling processes
};

// A job consists of a unique numeric ID and
// one or more processes
struct job {
  int id;
  struct kiddo *kidlets; // Linked list of child processes
  struct job *next; // Linked list of active jobs
};

// A singly linked list of active jobs.
static struct job *jobbies = NULL;

/* Initialize a job structure
 *
 * Returns an integer ID that represents the job.
 */
int create_job(void) {
  struct job *tmp;
  struct job *j = malloc(sizeof(struct job));
  j->id = ++job_counter;
  j->kidlets = NULL;
  j->next = NULL;
  if (jobbies) {
    for (tmp = jobbies; tmp && tmp->next; tmp = tmp->next) ;
    assert(tmp!=j);
    tmp->next = j;
  } else {
    jobbies = j;
  }
  return j->id;
}

/* Helper function to walk the job list and find
 * a given job.
 *
 * remove: If true, remove this job from the job list.
 *
 * Returns NULL on failure, a job pointer on success.
 */
static struct job *find_job(int job_id, bool remove) {
  struct job *tmp, *last = NULL;
  for (tmp = jobbies; tmp; tmp = tmp->next) {
    if (tmp->id == job_id) {
      if (remove) {
        if (last) {
          last->next = tmp->next;
        } else {
          assert (tmp == jobbies);
          jobbies = NULL;
        }
      }
      return tmp;
    }
    last = tmp;
  }
  return NULL;
}

/* Given the command listed in args,
 * try to execute it and create a job structure.
 *
 * This function does NOT wait on the child to complete,
 * nor does it return an exit code from the child.
 *
 * If the first argument starts with a '.'
 * or a '/', it is an absolute path and can
 * execute as-is.
 *
 * Otherwise, search each prefix in the path_table
 * in order to find the path to the binary.
 *
 * Then fork a child and pass the path and the additional arguments
 * to execve() in the child.  Wait for exeuction to complete
 * before returning.
 *
 * stdin is a file handle to be used for standard in.
 * stdout is a file handle to be used for standard out.
 *
 * If stdin and stdout are not 0 and 1, respectively, they will be
 * closed in the parent process before this function returns.
 *
 * job_id is the job_id allocated in create_job
 *
 * Returns 0 on success, -errno on failure to create the child.
 *
 */

int run_command(char *args[MAX_ARGS], int stdin, int stdout, int job_id) {
  /* Lab 2: Your code here */
  int rv = 0;
  int exist_flag=0;
  char fullpath[PATH_L];
  struct stat fileStat;
  int pipefd[2];
  pipe(pipefd);
  
  // if input is the full path of the command
  if(args[0][0]=='.'||args[0][0]=='/'){
    memset(fullpath,0,sizeof(char)*PATH_L);
    strcpy(fullpath,args[0]);
    exist_flag=1;
  }
  else{ // find the correct environment path of the command by using stat()
    for(int i=0;path_table[i];i++){
      memset(fullpath,0,sizeof(char)*PATH_L);
      snprintf(fullpath, sizeof(char)*PATH_L, "%s/%s", path_table[i], args[0]);
      if(stat(fullpath, &fileStat) == -1){
        continue;
      }
      else{
        exist_flag=1;
        break;
      }
    }
  }
  
  // exist_flag means finding the enviornment path
  if(exist_flag){
    if (stdin != STDIN_FILENO)
    {
      dup2 (stdin, STDIN_FILENO);
      close (stdin);
    }
    if (stdout != STDOUT_FILENO)
    {
      dup2 (stdout, STDOUT_FILENO);
      close (stdout);
    }

    // fork a child
    pid_t child_pid = fork();

    if (child_pid == 0) { // Child process
      close(pipefd[1]);
      close(pipefd[0]);
      execv(fullpath, args);
      exit(EXIT_FAILURE);
    }
    else if (child_pid > 0) { // Parent process
      // add the child to the job list
      struct job* j_now=find_job(job_id,false);
      struct kiddo* k=malloc(sizeof(struct kiddo));
      k->pid=child_pid;
      k->next=NULL;
      struct kiddo* k_temp=j_now->kidlets;
      while (k_temp!=NULL)
      {
        k_temp=k_temp->next;
      }
      k_temp=k;
      int status;
      waitpid(child_pid, &status, 0);
    }
    else { // Fork failed
        dprintf(2, "Fork error\n");
        return -errno;
    }
  }
  else{
    // can't find the enviornment path of the command
    dprintf(stdout,"Failed to run command - error -2\n");
    dprintf(2,"Invalid command!\n");
  }

  // Suppress the compiler warning that find_job is not used in the starer code.
  // You may remove this line if/when you use find_job in your code.
  // (void)&find_job;
  return rv;
}

/* Wait for the job to complete and free internal bookkeeping
 *
 * job_id is the job_id allocated in create_job
 *
 * exit_code is the exit code from the last child process, if it executed.
 *           This parameter may be NULL, and is only set if the return
 *           value is zero.  This is the same as the wstatus parameter
 *           to waitpid variants, and can be used with functions such
 *           as WIFEXITED.  If this job includes multiple
 *           processes, the exit code will be the last process.
 *
 * Returns zero on success, -errno on error.
 */
int wait_on_job(int job_id, int *exit_code) {
  int ret = 0;
  return ret;
}
