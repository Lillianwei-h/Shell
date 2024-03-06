/* COMP 530: Tar Heel SHell */
// PID:730723179
// PID:730723162

#include "thsh.h"

#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

int main(int argc, char **argv, char **envp) {
  // flag that the program should end
  bool finished = 0;
  int input_fd = 0; // Default to stdin
  int ret = 0;


  // Lab 2:
  // Add support for parsing the -d option from the command line
  // and handling the case where a script is passed as input to your shell

  // Lab 2: Your code here
  int d_flag=0;
  int s_flag=0;
  FILE *inputFile=NULL;
  if (argc == 2) {
    if(strcmp(argv[1],"-d")==0){ // if debug
      d_flag=1;
    }
    else {	// if script
      s_flag=1;
      // get the script and put its handle in input_fd
      char *inputFileName = argv[1];
      inputFile = fopen(inputFileName, "r");
      if (inputFile == NULL) {
          dprintf(2, "Error opening input file");
          return 1;
      }
      input_fd=fileno(inputFile);
    }
  }
	
  // initialize current working dir
  ret = init_cwd();
  if (ret) {
    dprintf(2, "Error initializing the current working directory: %d\n", ret);
    return ret;
  }
  // initialize [ath table
  ret = init_path();
  if (ret) {
    dprintf(2, "Error initializing the path table: %d\n", ret);
    return ret;
  }

  // keep running our shell
  while (!finished) {

    int length;
    // Buffer to hold input
    char cmd[MAX_INPUT];
    // Buffer for scratch space - optional, only necessary for challenge problems
    char scratch[MAX_INPUT];
    // Get a pointer to cmd that type-checks with char *
    char *buf = &cmd[0];
    char *parsed_commands[MAX_PIPELINE][MAX_ARGS];
    char *infile = NULL;
    char *outfile = NULL;
    int pipeline_steps = 0;
	
    if (!input_fd) {
      ret = print_prompt();
      if (ret <= 0) {
        // if we printed 0 bytes, this call failed and the program
        // should end -- this will likely never occur.
        finished = true;
        break;
      }
    }

    // Reset memory from the last iteration
    for(int i = 0; i < MAX_PIPELINE; i++) {
      for(int j = 0; j < MAX_ARGS; j++) {
        parsed_commands[i][j] = NULL;
      }
    }

    // Read a line of input
    length = read_one_line(input_fd, buf, MAX_INPUT);
    if (length <= 0) {
      ret = length;
      break;
    }

    // Add it to the history
    add_history_line(buf);

    // Pass it to the parser
    pipeline_steps = parse_line(buf, length, parsed_commands, &infile, &outfile, scratch, MAX_INPUT);
    if (pipeline_steps < 0) {
      dprintf(2, "Parsing error.  Cannot execute command. %d\n", -pipeline_steps);
      continue;
    }

    int in_fd = 0;
    int out_fd = 1;
    // if our pipeline has an input or output
    if(infile){
      in_fd = open(infile, O_RDONLY);
    }
    if(outfile){
      out_fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    }
    int prev_output=in_fd;
    int cur_output=out_fd;
    int pipefd[2];
    // save stdin and stdout
    int original_stdin = dup(STDIN_FILENO); 
    int original_stdout = dup(STDOUT_FILENO);

    // Just echo the command line for now
    // file descriptor 1 -> writing to stdout
    // print the whole cmd string (write number of
    // chars/bytes equal to the length of cmd, or MAX_INPUT,
    // whichever is less)
    //
    // Comment this line once you implement
    // command handling
    // dprintf(1, "%s\n", cmd);

    int job_id=create_job();
    for(int i=0;parsed_commands[i][0];i++){
      pipe(pipefd); // get current pipe's file descriptor for reading and writing
      if(!parsed_commands[i+1][0]){
	// if this is the last command of the pipeline, cur_output is out_fd (either stdout or our output file)
        cur_output = out_fd;
      }
      else{
	// if this is not last command of the pipeline, cur_output is the writing fd of the pipe
        cur_output = pipefd[1];
      }
	    
      if(d_flag){ // if debug
        dprintf(2, "RUNNING: [%s]\n", parsed_commands[i][0]);
      }
	    
      if(!handle_builtin(parsed_commands[i],prev_output,cur_output,&ret)){ // if is not a builtin command
        ret=run_command(parsed_commands[i],prev_output,cur_output,job_id); //run
        if(d_flag){ // print debug message
          dprintf(2, "ENDED: [%s] (ret=%d)\n", parsed_commands[i][0], ret);
        }
        if(ret==0){
          // dprintf(1, "Success!\n");
        }
        else{ // running error
          dprintf(2, "Failure to create the child!\n");
        }
      }
      else{ // if is a builtin command
        if(d_flag){ // print debug message
          dprintf(2, "ENDED: [%s] (ret=%d)\n", parsed_commands[i][0], ret);
        }
        if(ret==0){
          // dprintf(1, "Success!\n");
        }
        else{ // running error
          dprintf(2, "Failure to create the child!\n");
        }
      }
      close(pipefd[1]); // close writing handle
      prev_output = pipefd[0]; // save the current reading handle as the previous command's output handle, working as the next command's input
    }
    // close input and output file's handle
    if(infile){
      close(in_fd);
    }
    if(outfile){
      close(out_fd);
    }
	  
    // resume stdin and stdout
    dup2(original_stdin, STDIN_FILENO);
    dup2(original_stdout, STDOUT_FILENO);
    close(original_stdin);
    close(original_stdout);


    // In Lab 2, you will need to add code to actually run the commands,
    // add debug printing, and handle redirection and pipelines, as
    // explained in the handout.
    //
    // For now, ret will be set to zero; once you implement command handling,
    // ret should be set to the return from the command.
    // ret = 0;

    // Do NOT change this if/printf - it is used by the autograder.
    if (ret) {
      char buf [100];
      int rv = snprintf(buf, 100, "Failed to run command - error %d\n", ret);
      if (rv > 0)
	      write(1, buf, strlen(buf));
      else
	      dprintf(2, "Failed to format the output (%d).  This shouldn't happen...\n", rv);
    }

  }
  if(s_flag){
      fclose(inputFile);
  }

  // Only return a non-zero value from main() if the shell itself
  // has a bug.  Do not use this to indicate a failed command.
  return 0;
}
