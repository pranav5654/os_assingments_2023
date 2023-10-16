#include "os3.h"

struct prompt {
    char* command;
    int pid;
    char * date;
    int exec_time;
    int priority;
    int wait_time;
    struct prompt * next;
};


struct process_table{

    struct pcb * head;
    struct pcb * tail;
    int size;
    sem_t mutex;

};

struct pcb{
    int pid;
    char f_name[20];
    char * date;
    struct pcb * next;
    int execution;
    int wait;
    int priority;
};



// need to to do all the error checkign ---------------- done
// default priority 1 ---------------------------------- done
// priority queue -------------------------------------- done
// add original shell code ----------------------------- done 
// cant quit if all the processes havent endede -------- done --- need to update if commands can run in between
// histrory -------------------------------------------- done
// need to change the ctrl c functionality ------------- done
// add return to all the perror ------------------------ done
// add waittime ---------------------------------------- done
// if adding process while something is running.


struct prompt * head = NULL;
struct prompt * tail = NULL;
int num =0;
int flag =0 ;

char * shm_name = "/process table";
struct process_table* shared;
int fd;
int TSLICE;
int NCPU;


void timer_handler(int signum){

    if(signum == SIGINT) {
        char buff1[32] = "\n\nCaught SIGINT(ctrl C) signal\n\n";
        write(STDOUT_FILENO, buff1, 32);
        print_history();
        printf("%s\n", "\033[1;31mCODE TERMINATED\033[0m");
        exit(0);
    } 

    if(flag ==0) {return;}

    if(signum == SIGALRM) scheduler();
}






int main(int argc, char const *argv[]){        
    if (argc != 3) {
        printf("Usage: %s NCPU TSLICE\n", argv[0]);
        return 1;
    }

    NCPU = atoi(argv[1]);
    TSLICE = atoi(argv[2]);

    fd = shm_open(shm_name , O_RDWR, S_IRUSR | S_IWUSR);
    if(fd ==-1){
        printf("%s\n", "Error in shm_open");
        exit(0);   
    }

    shared = (struct process_table *)mmap(NULL , sizeof(struct process_table) , PROT_READ | PROT_WRITE, MAP_SHARED, fd , 0);
    
    if(shared == MAP_FAILED){
        printf("%s\n", "Error in mmap");
        exit(0);
    }

    int check = sem_init(&shared->mutex , 1,1);
    if(check ==-1){perror("sem_init failed"); exit(-1);}

    
    struct sigaction sig;
    struct itimerval timer;

    sig.sa_handler = timer_handler;
    sig.sa_flags = SA_RESTART;
    sigaction(SIGALRM, &sig, NULL);
    sigaction(SIGINT, &sig, NULL);

    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = TSLICE*1000;
    timer.it_interval = timer.it_value;

    setitimer(ITIMER_REAL, &timer, NULL);

    shell_loop();
    printf("%s\n", "\033[1;31mCODE TERMINATED\033[0m");

    munmap(shared , sizeof(struct process_table));
    close(fd);
    sem_destroy(&shared->mutex);
    shm_unlink(shm_name);
    return 0;
}
    


void scheduler(){

    int ti = 1;
    struct pcb* temp_w;
    sem_wait(&shared->mutex);   
    temp_w = shared->head;
    sem_post(&shared->mutex);   

    while(temp_w != NULL){
        if(ti <= NCPU) {
            ti ++;
            temp_w = temp_w->next;
            continue;
        }
        temp_w->wait += TSLICE;
        ti ++;
        temp_w = temp_w->next;
    }

    struct pcb * storage[NCPU];

    for (int i = 0; i < NCPU; i++){
            
        struct pcb * t;
        sem_wait(&shared->mutex);    
        t = shared->head;
        sem_post(&shared->mutex);   

        if(t == NULL){storage[i] = NULL;continue;}

        
        int status = fork();

        if(status < 0){perror("fork failed"); return;}

        else if(status == 0){     

            sem_wait(&shared->mutex);    
            pid_t pid = shared->head->pid;
            sem_post(&shared->mutex);   

            int a = TSLICE*1000;

            kill(pid , SIGCONT);
            usleep(a);
            kill(pid , SIGSTOP);  
 
            exit(0);
        }
            
        else{   
            int ret;
            waitpid(status , &ret ,0);                
            sem_wait(&shared->mutex);    
            pid_t pid =shared->head->pid;
            sem_post(&shared->mutex);   
            
            int s;
            int result = waitpid( pid, &s , WNOHANG);

            if(result ==0 ){
                sem_wait(&shared->mutex);    
                shared->head->execution += TSLICE;
                sem_post(&shared->mutex);   
                storage[i] = dequeue();                   
            }

            else{
                
                sem_wait(&shared->mutex);    
                shared->head->execution = TSLICE + shared->head->wait;
                sem_post(&shared->mutex);   
                // add the thing to history as it has stopped.
                struct pcb* n;
                n = dequeue();
                history(n->f_name , n->pid , n->priority , n->date , n->execution , n->wait);

                // print_pcbs();
                free(n);
                storage[i] = NULL;

                
            }
        
            
        }
    
    
    }   


    for (int i = 0; i < NCPU; i++)
    {
        if(storage[i] == NULL) continue;
        struct pcb* temp = storage[i];
        struct pcb* t;
        temp->next = NULL;
        t = enqueue(temp);
        // print_pcbs();
        
        sem_wait(&shared->mutex);      
        shared->head = t;
        sem_post(&shared->mutex);  
        
    }

        
    struct pcb* new_temp;
    sem_wait(&shared->mutex);    
    new_temp = shared->head; 
    sem_post(&shared->mutex);    

    if(new_temp == NULL){
        flag=0;
    }
        
        
}



void shell_loop(){

    char command[1024];
    do{
        printf("\033[1;35mOS_assignment_3\033[0m$ ");

        fgets(command , 1024 ,stdin); 
        
        if(strcmp(command , "\n") == 0 || strcmp(command , " \n") == 0){continue;}

        command[strlen(command) -1] = '\0';

        if(strcmp(command , "exit") ==0){break;}

        else if(strcmp(command , "history") == 0){
            print_history();
        }
        else if(strcmp(command , "print") == 0){          
            print_pcbs();
        }
        else if(strcmp(command , "start") == 0){
            flag =1;
        }

        
        else{
            int status = create_n_run(command);
        }
        
        
    } while (1);
}


struct pcb* new_process(char * command , int p){
    
    struct pcb * process  = (struct pcb *)malloc(sizeof(struct pcb));
    process->pid = -1;
    process->execution =0;
    process->wait  =0;
    process->priority = p;
    strncpy(process->f_name , command , 20);

    pid_t pid = fork();

    if(pid <0){perror("fork failed");return NULL;}

    else if(pid ==0){
        execlp(process->f_name , process->f_name , NULL);
        perror("exec failed");
        exit(1);}
        
    else {
        
        kill(pid , SIGSTOP);
        time_t t;
        time(&t);
        char * timee = ctime(&t);
        timee[strlen(timee) -1] = '\0';
        process->date = timee;
        process->pid = pid;
        return enqueue(process);

    }

}

int create_n_run(char * command){
    char * c_temp = strdup(command);
    char * arguments[100];
    int i =0;      
    char * word = strtok(command , " ");
    
    while(word != NULL){
        arguments[i] = word;
        i++;
        word = strtok(NULL , " ");
    } 
    arguments[i] = NULL;
    

    if(strcmp(arguments[0] , "submit") == 0){
        if(i>3 || i<2){
            return -1;
        }
        int k;
        if(i==3){
            k = atoi(arguments[2]);
        }
        else if(i==2) k=1;

        struct pcb*temp = new_process(arguments[1] , k);
        sem_wait(&shared->mutex);   
        shared->head = temp;
        sem_post(&shared->mutex);     
        time_t t;
        time(&t);
        char * timee = ctime(&t);
        timee[strlen(timee) -1] = '\0'; 

        history(c_temp , -1 ,-1, timee , 0 ,0);


        return 0;
    }
    

    
    int status =fork();
    if(status < 0) {perror("fork failed"); return -1;}

    else if (status ==0){

        if(execvp(arguments[0] , arguments) == -1){
            printf("Invalid Input!!!\n");
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
        history(c_temp , status ,-1, timee , exec_time*1000 ,0);

        // returning exit code
        return WEXITSTATUS(ret);
    }
}





void history(char * command , int pid ,int priority,  char * date , int exec_time , int wait_time){
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
        newnode->wait_time = wait_time;
        newnode->priority = priority;
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
    newnode->wait_time = wait_time;
    newnode->priority = priority;
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
        printf("%d.  %-20s  (PID : %-7d)  %5s   CompTime : %-7d ms    WaitTime : %-7d ms     Priority:   %d\n", i , temp->command , temp->pid , temp->date , temp->exec_time ,temp->wait_time , temp->priority);
        i++;
        temp = temp->next;
    }
}




struct pcb* push(struct pcb * process)  
{  
    int p =process->priority;
    struct pcb * temp;
    sem_wait(&shared->mutex);  
    temp  = shared->head;
    sem_post(&shared->mutex);   

    if (temp->priority > p) {  

        process->next = temp;  
        sem_wait(&shared->mutex);   
        shared->head = process;
        sem_post(&shared->mutex);      

    }  
    else {  
  
        while (temp->next != NULL &&  temp->next->priority <= p) {  
            temp = temp->next;  
        }   
        process->next = temp->next;  
        temp->next = process;
        sem_wait(&shared->mutex);   
        struct pcb * new = shared->head;
        sem_post(&shared->mutex);    
        return new;
    }  
}  

struct pcb * enqueue(struct pcb * process){
    
    sem_wait(&shared->mutex);     
    
    if(shared->head == NULL){
        shared->head = process;
        shared->tail = process;
        shared->size ++;
        sem_post(&shared->mutex);   

        return shared->head;

    }

    else{    
        sem_post(&shared->mutex);   
        return push(process);   
    }
}

struct pcb * dequeue(){
    struct pcb* t;
    sem_wait(&shared->mutex);  
    t = shared->head;
    sem_post(&shared->mutex);   
    if(t == NULL) {return NULL;}
    
    struct pcb * temp =t;
    sem_wait(&shared->mutex);   
    shared->head = shared->head->next;
    sem_post(&shared->mutex);   
    return temp;
}

void print_pcbs(){ 
    
    sem_wait(&shared->mutex);    
    struct pcb * temp  =shared->head;
    sem_post(&shared->mutex);     

    printf("\n%s\n", "###########################################################");
    printf("%s\n", "PID      NAME     PRIORITY     EXECUTION");
    
    
    while(temp != NULL){
        printf("%d    %s       %d              %d\n" , temp->pid , temp->f_name ,temp->priority,  temp->execution);
        temp = temp->next;
    }

    printf("%s\n\n","############################################################");
}


