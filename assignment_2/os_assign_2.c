#include<stdio.h>
#include<string.h>
#include<stdbool.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<time.h>
#include<signal.h>

struct prompt {
    char* command;
    int pid;
    char * date;
    int exec_time;
    struct prompt * next;
};


struct prompt * head = NULL;
struct prompt * tail = NULL;
int num =0;

int create_n_run_pipe(char * command);
int create_n_run_pipes(char * command);
static void my_handler(int signum);
void shell_loop();
int create_n_run(char * command);
int launch(char * command);
void history(char * command , int pid , char * date , int exec_time);
void print_history();


int main(){
    // making sa_handler for getting ctrl c function
    struct sigaction sig;
    memset(&sig, 0, sizeof(sig));
    sig.sa_handler = my_handler;
    sigaction(SIGINT, &sig, NULL);
    

    shell_loop();
    
    print_history();
    
    
    printf("%s\n", "\033[1;31mCODE TERMINATED\033[0m");

    
    return 0;

}


void shell_loop(){
    char command[1024];

    // infinite whle loop for the shell
    do
    {
        printf("\033[1;35mOS_assignment_2\033[0m$ ");
        fgets(command , 1024 ,stdin);  // getting input
        command[strlen(command) -1] = '\0'; // removing the newlinr character from the input

        // exit condition
        if(strcmp(command , "exit") ==0){
            break;
        }

        // history condition
        else if(strcmp(command , "history") == 0){
            print_history();
            continue;
        }

        int status = launch(command);   // calling the launch function
        
    } while (1);
}


int launch(char * command){

    // launches the create_n_run function for creating a child process.
    int status;
    int count =0;
    for (int i = 0; i < strlen(command); i++)
    {
        if(command[i] == '|'){
            count ++;
        }
    }
    // detecting if .sh file or not
    for (int i = 0; i < strlen(command) -2; i++)
    {
        if(command[i] =='.' && command[i+1] == 's' && command[i+2] == 'h'){
            count =2 ;
        }
    }
    
    if(count ==0){
        status = create_n_run(command);
    }
    else if(count ==1){
        status = create_n_run_pipe(command);
    }
    else{
        status = create_n_run_pipes(command);
    }
    return status;
}


int create_n_run(char * command){
    int status = fork();

    if(status < 0){
        printf("%s\n", "Something went wrong");
        printf("%s\n", "Fork failed.");                     // gives an output to console 
    }
    // child process will run this 
    else if (status ==0){

        // gettting space serated argumnents
        char * arguments[100];
        int i =0;      
        char * word = strtok(command , " ");
        
        while(word != NULL){
            arguments[i] = word;
            i++;
            word = strtok(NULL , " ");
        } 
        // making the last argument NULL as the exec commands needa list in which the last argument is NULL pointer
        arguments[i] = NULL;

        // creating a string for execv
        char  a[100];
        strcpy(a , "/usr/bin/");
        strcat(a , arguments[0]);
        // calling the execv function and checking error
        if(execv(a , arguments) == -1){
            printf("Invalid Input!!!\n");     // gives an output to console 
            exit(1);
        }
        exit(0);

    }
    else{
        // parent process first waits for the child to end and then executes its code
        int ret;
        time_t s_timer;
        time_t e_timer;
        time(&s_timer);
        wait(&ret); // waiting for the child
        time(&e_timer);
        int exec_time = e_timer - s_timer;
    
        // prints the exit code of the child if it terminated correctly
        if (WIFEXITED(ret)) {
            printf("EXIT CODE of the CHILD: %d\n", WEXITSTATUS(ret));
            
        }

        // storing the history of the command
        time_t t;
        time(&t);
        char * timee = ctime(&t);
        timee[strlen(timee) -1] = '\0';    
        history(command , status , timee , exec_time*1000);

        // returning exit code
        return WEXITSTATUS(ret);
    }
}

// stores the history provided
void history(char * command , int pid , char * date , int exec_time){
    if(head == NULL){

        // making a newnode for the head
        struct prompt * newnode = NULL;
        newnode = (struct prompt *)malloc(sizeof(struct prompt));

        // duplicating the strings of command and date  to store the current copy
        newnode->command = strdup(command);
        newnode->date = strdup(date);
        
        // assigning values to newnnode created
        newnode->pid = pid;
        newnode->exec_time = exec_time;
        newnode->next = NULL;
        head = newnode;
        tail = head;
        num++;
        return;
    }

    // making a newnode to add to existing ones
    struct prompt * newnode = NULL;
    newnode = (struct prompt *)malloc(sizeof(struct prompt));
    
    // duplicating the strings of command and date  to store the current copy
    newnode->command = strdup(command);
    newnode->date = strdup(date);

    // assigning values to newnnode created
    newnode->pid = pid;
    newnode->exec_time = exec_time;
    newnode->next = NULL;
    tail->next = newnode;
    tail = newnode;
    num++;

    // shifting the head one forward if commands are more than hundread
    if(num > 100){
        struct prompt * temp = head;
        head = head->next;
        free(temp);
    }
    return;
}


void print_history(){
    struct prompt * temp = head;
    int i = 1;
    while(temp != NULL){
        printf("%d.  %-10s  (PID : %d)  %5s   ExecTime : %d ms\n", i , temp->command , temp->pid , temp->date , temp->exec_time);
        i++;
        temp = temp->next;
    }
}


// handles the signal ctrl c and then terimnates , print the history
static void my_handler(int signum) {
    if(signum == SIGINT) {
        char buff1[32] = "\n\nCaught SIGINT(ctrl C) signal\n\n";
        write(STDOUT_FILENO, buff1, 32);
        print_history();
        printf("%s\n", "\033[1;31mCODE TERMINATED\033[0m");
        exit(0);
    } 
    
}


int create_n_run_pipes(char * command){
    FILE *fp;
    char words[1024];

    // Open a pipe to a command
    fp = popen(command, "r");
    while (fgets(words, sizeof(words), fp) != NULL) {
        printf("%s", words);
    }
    // closing the file pointer
    pclose(fp);
}


int create_n_run_pipe(char * command){
    int fd[2];

    if(pipe(fd) == -1){
        printf("Problem while forming pipe\n");
        return 0;
    }

    // creating commands seprated by pipes
    int k =0;
    char * commands[100];
    char * c = strtok(command , "|");
    while(c != NULL){
        commands[k] = c;
        k++;
        c = strtok(NULL , "|");
    }
    
    
    int status = fork();

    if(status < 0){
        printf("%s\n", "Something went wrong");
        printf("%s\n", "Fork failed.");
           // gives an output to console 
    }
    else if(status == 0){

        close(fd[0]); // closing the reading part of the pipe

        dup2(fd[1] , STDOUT_FILENO);
        close(fd[1]);

        char * command = commands[0];
        char * arguments[100];
        int i =0;      
        char * word = strtok(command , " ");
        
        while(word != NULL){
            arguments[i] = word;
            i++;
            word = strtok(NULL , " ");
        } 
        // making the last argument NULL as the exec commands needa list in which the last argument is NULL pointer
        arguments[i] = NULL;
        // creating a string for execv
        char  a[100];
        strcpy(a , "/usr/bin/");
        strcat(a , arguments[0]);
        
        if(execv(a ,arguments) == -1){
            printf("Invalid Input!!!\n");     // gives an output to console 
            exit(1);
        }
        exit(0);


        
    }
    else{

        int status2 = fork();

        if(status2 < 0){
            printf("%s\n", "Something went wrong");
            printf("%s\n", "Fork failed.");
               // gives an output to console 
        }
        else if(status2 == 0){
            close(fd[1]); // closing the reading part of the pipe

            dup2(fd[0] , STDIN_FILENO);
            close(fd[0]);

        char * command = commands[1];
        char * arguments[100];
        int i =0;      
        char * word = strtok(command , " ");
        
        while(word != NULL){
            arguments[i] = word;
            i++;
            word = strtok(NULL , " ");
        } 
        // making the last argument NULL as the exec commands needa list in which the last argument is NULL pointer
        arguments[i] = NULL;

        // creating a string for execv
        char  a[100];
        strcpy(a , "/usr/bin/");
        strcat(a , arguments[0]);
        
        if(execv(a , arguments) == -1){
            printf("Invalid Input!!!\n");     // gives an output to console 
            exit(1);
        }
        exit(0);
        }





        else{
            // parent process
            close(fd[0]);
            close(fd[1]);
            wait(NULL);
            wait (NULL);
        }
    }
    
    return 0;

}


