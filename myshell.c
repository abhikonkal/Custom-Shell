/********************************************************************************************
This is a template for assignment on writing a custom Shell. 

Students may change the return types and arguments of the functions given in this template,
but do not change the names of these functions.

Though use of any extra functions is not recommended, students may use new functions if they need to, 
but that should not make code unnecessorily complex to read.

Students should keep names of declared variable (and any new functions) self explanatory,
and add proper comments for every logical step.

Students need to be careful while forking a new process (no unnecessory process creations) 
or while inserting the single handler code (should be added at the correct places).

Finally, keep your filename as myshell.c, do not change this name (not even myshell.cpp, 
as you not need to use any features for this assignment that are supported by C++ but not by C).
*********************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>			// exit()
#include <unistd.h>			// fork(), getpid(), exec()
#include <sys/wait.h>		// wait()
#include <signal.h>			// signal()
#include <fcntl.h>			// close(), open()

#define TRUE 1
#define FALSE 0 //using TRUE and FALSE instead of 1 and 0 for better readability

//using the following global variables to store the command line arguments
size_t MAX_BUFFER = 200; //maximum size of the buffer
size_t DEL_SIZE = 3;//size of the delimiter
size_t MAX_CMD_LEN = 30;//maximum length of the command
size_t MAX_CMDS = 5;//maximum number of commands
size_t MAX_FILE_SIZE = 128;//maximum size of the file name


void executeCommandRedirection(char** argv, int argc, char* del, int flag);
void executeCommand(char** argv, int waitprflag, int redirectflagval);
void executeParallelCommands(char** argv, int argc, const char* del);



// This function will parse the input string into multiple commands or a single command with arguments depending on the delimiter (&&, ##, >, or spaces).
char** parseInput(char** inputStr, int* n, char** del)
{
    char* str = (char*)malloc(sizeof(char)*MAX_BUFFER);
    strcpy(str, *inputStr);
    int count=0, len=strlen(str);

    
    for(int i=0; i<len; i++) {
        if(str[i] == ' ')
            count++;
    }
    int argc = count+2;             // Number of arguments = Number of delimiters + 1; One extra for last argument = NULL
    char** argv = (char**)malloc(sizeof(char*)*argc);//array of strings to store the commands
    for(int i=0; i<argc-1; i++) {
        argv[i] = (char*)malloc(sizeof(char)*MAX_CMD_LEN);
        strcpy(argv[i], strsep(&str, *del));
    }
    argv[argc-1] = (char*)NULL;


    for(int i=1; i<argc-1; i++) {
        if(!strcmp(argv[i], "&&")) {
            strcpy(*del, "&&");
            break;
        }
        else if(!strcmp(argv[i], "##")) {
            strcpy(*del, "##");
            break;
        }
        else if(!strcmp(argv[i], ">")) {
            strcpy(*del, ">");
        }
    }

    *n = argc;
    return argv;
}

// This function will fork a new process to execute a command
//
void executeCommand(char** argv, int waitprflag, int redirectflagval) 
{
    if(!strcmp(argv[0], "cd")) {
        if(chdir(argv[1])!=0) {
            printf("No such directory!\n");
        }
        return;
    }
	int rc = fork();
    if(rc<0) {  // Fork Failed
        fprintf(stderr, "Fork Failed\n");
        exit(1);
    }
    else if(rc==0) {    // Child Process

        signal(SIGINT, SIG_DFL);//default
        signal(SIGTSTP, SIG_DFL); 

        if(redirectflagval) {
            int i=0;
            while(argv[i] != NULL) {
                i++;
            }
            i--;
            close(STDOUT_FILENO);
            open(argv[i], O_CREAT | O_RDWR | O_APPEND, 0666);
            argv[i] = NULL;
            if(execvp(argv[0], argv) == -1) {
                printf("Shell: Incorrect command\n");
                exit(1);
            }
        }
        else {
            if(execvp(argv[0], argv) == -1) {
                printf("Shell: Incorrect command\n");
                exit(1);
            }
        }
        
    }
    else {  // Parent Process
        if(waitprflag) {
            int rc_wait = waitpid(rc, NULL, WUNTRACED);
            //int rc_wait = wait(NULL);
        }
    }

}

// This function will run multiple commands in parallel
void executeParallelCommands(char** argv, int argc, const char* del)
{
    char** arr = (char**)malloc(sizeof(char*)*MAX_CMDS);

    int del_count=0;
    for(int i=0; i<argc-1; i++) {
        if(!strcmp(argv[i], del)) {
            del_count++;
        }
    }//counting the number of delimiters

    int i, j=0, k, count=0;
    int redirectflagval;//flag to check if redirection is required
    for(i=0; i<del_count+1; i++) {
        for(k=0; k<MAX_CMDS; k++) {
            arr[k] = (char*)malloc(sizeof(char)*MAX_CMD_LEN);
        }
        k=0;
		redirectflagval = FALSE;
        while(j < argc-1 && strcmp(argv[j], del)) {
            strcpy(arr[k], argv[j]);
            if(redirectflagval == FALSE && !strcmp(arr[k], ">")) {
                redirectflagval = TRUE;
            }
            j++; k++;
        }
        arr[k] = NULL; j++;

        if(redirectflagval) {
            //if redirection is required, then the last argument is the file name
            char* delim = (char*)malloc(sizeof(char)*DEL_SIZE);
            strcpy(delim, ">");
            executeCommandRedirection(arr, k+1, delim, 0);
        }
        else {
            //if redirection is not required, then execute the command
            executeCommand(arr, FALSE, FALSE);   // No wait for Parallel
        }

        for(k=0; k<MAX_CMDS; k++) {
            free(arr[k]);
        }
    }
    for(i=0; i<del_count+1; i++) {
        int rc_wait = wait(NULL);
    }
}

// This function will run multiple commands in parallel
void executeSequentialCommands(char** argv, int argc, char* del)
{	
    //this function is similar to the above function, except that it waits for the child process to finish before executing the next command
    char** arr = (char**)malloc(sizeof(char*)*MAX_CMDS);

    int del_count=0;
    for(int i=0; i<argc-1; i++) {
        if(!strcmp(argv[i], del)) {
            del_count++;
        }
    }//counting the number of delimiters

    int i, j=0, k, count=0;
    int redirectflagval;//flag to check if redirection is required
    for(i=0; i<del_count+1; i++) {
        for(k=0; k<MAX_CMDS; k++) {
            arr[k] = (char*)malloc(sizeof(char)*MAX_CMD_LEN);
        }
        k=0;redirectflagval = FALSE;
        while(j < argc-1 && strcmp(argv[j], del)) {
            strcpy(arr[k], argv[j]);
            if(redirectflagval == FALSE && !strcmp(arr[k], ">")) {
                redirectflagval = TRUE;
            }
            j++; k++;
        }
        arr[k] = NULL; j++;

        if(redirectflagval) {
            char* delim = (char*)malloc(sizeof(char)*DEL_SIZE);
            strcpy(delim, ">");
            executeCommandRedirection(arr, k+1, delim, 0);
        }
        else {
            executeCommand(arr, TRUE, FALSE);   // Wait for sequential
        }

        for(k=0; k<MAX_CMDS; k++) {
            free(arr[k]);
        }
    }

}
// This function will run a single command with output redirected to an output file specificed by user
// This function will also run multiple commands in parallel with output redirected to an output file specificed by user
void executeCommandRedirection(char** argv, int argc, char* del, int flag)  // flag = 0 if should not wait, else 1 
{
    char** arr = (char**)malloc(sizeof(char*)*MAX_CMDS);

    int del_count=0;
    for(int i=0; i<argc-1; i++) {
        if(!strcmp(argv[i], del)) {
            del_count++;
        }//counting the number of delimiters
    }

    if(del_count != 1) {
        fprintf(stderr, "Incorrect No of Arguments,few/many arguments!\nUsage: command > file\n");
        return;
    }

    int i=0;
    //finding the index of the delimiter
    while(i < argc-1 && strcmp(argv[i], del)) {
        arr[i] = (char*)malloc(sizeof(char)*MAX_CMD_LEN);
        strcpy(arr[i], argv[i]);
        i++;
    }
    arr[i] = (char*)malloc(sizeof(char)*MAX_CMD_LEN);
    //the next argument is the file name
    stpcpy(arr[i], argv[i+1]);
    arr[i+1] = (char*)malloc(sizeof(char)*MAX_CMD_LEN);
    arr[i+1] = NULL;
    
    if(flag == 0) {
        //if flag is 0, then execute the command without waiting
        executeCommand(arr, FALSE, TRUE);
    } else {
        //if flag is 1, then execute the command and wait for it to finish
        executeCommand(arr, TRUE, TRUE);
    }
}

int main()
{
	// Initial declarations
    char* presworkingdirectory = (char*)malloc(sizeof(char)*MAX_BUFFER);
    char* inputStr = (char*)malloc(sizeof(char)*MAX_BUFFER);
    char** argv;
    int argc=0;
    char* del = (char*)malloc(sizeof(char)*DEL_SIZE);
	
	while(1)	// Infinite loop until user exits
	{
       
        //Printing prompt on shell
        getcwd(presworkingdirectory, MAX_BUFFER);
		printf("%s$",presworkingdirectory);

        signal(SIGINT, SIG_IGN);    // Ignore the signal ctrl c
        signal(SIGTSTP, SIG_IGN);   // Ignore the signal ctrl z
		
        getline(&inputStr, &MAX_BUFFER, stdin);
        //strcpy(inputStr, "ls > output.txt\n");
        int n = strlen(inputStr);
        if(n == 1) {continue;}
        inputStr[n-1] = '\0';
        

		// Parse input with 'strsep()' for different symbols (&&, ##, >) and for spaces.
        strcpy(del, " ");
		argv = parseInput(&inputStr, &argc, &del);

		
		if(!strcmp(argv[0], "exit"))	// When user uses exit command.
		{
			printf("Exiting shell...\n");
			break;
		}
		
		if(!strcmp(del, "&&"))
			executeParallelCommands(argv, argc, del);		// This function is invoked when user wants to run multiple commands in parallel (commands separated by &&)
		else if(!strcmp(del, "##"))
			executeSequentialCommands(argv, argc, del);	// This function is invoked when user wants to run multiple commands sequentially (commands separated by ##)
		else if(!strcmp(del, ">"))
			executeCommandRedirection(argv, argc, del, 1);	// This function is invoked when user wants redirect output of a single command to and output file specificed by user
		else
			executeCommand(argv, TRUE, FALSE);		// This function is invoked when user wants to run a single command
				
	}
	
	return 0;
}
