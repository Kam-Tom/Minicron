#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <syslog.h>
#include <sys/wait.h>
#include "task.h"
#include <signal.h>
#include <stdatomic.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
//Define
#define NO_SIG 0
#define RELOAD_SIG 1
#define LOG_SIG 2
#define TERMINATE_SIG 3

//Functions declarations
void read_file(int);
int min_dif(Task*);
int sec_now();
void deamon_loop();
void start_task(Task*);
int create_first_process(List*,int);
void create_last_process(List*,int,int);
int create_middle_process(List*,int,int);
void create_one_process(List*,int);
void output_mode(int,bool);
void reload_list();
void terminate_all();
void signal_handler_reload_tasks();
void signal_handler_log_tasks();
void signal_handler_terminate_all();
void create_observer_process(char*);
void set_default_signals();
void daemonize();
//Global var
List* list;
int outfile_fd;
int taskfile_fd;
char* taskfile_filename;
atomic_char irruption_code;



int main(int argc, char* argv[])
{

    switch(argc)
    {
        case 1:
            perror("minicron: Demon uruchamiany w sposób następujący ./minicron <taskfile> <outfile>\n");
            exit(EXIT_FAILURE);
        case 2:
            perror( "minicron: Demon uruchamiany w sposób następujący ./minicron <taskfile> <outfile>\n");
            exit(EXIT_FAILURE);
    }

    daemonize();
    
    taskfile_fd = open(argv[1],O_RDONLY);
    if(taskfile_fd == -1)
    {
        perror("minicron: nie mozna otworzyc pliku <taskfile>");
        exit(EXIT_FAILURE); 
    }
    outfile_fd = open(argv[2],O_WRONLY | O_CREAT | O_APPEND,0644);
    if(outfile_fd == -1)
    {
        perror("minicron: nie mozna stworzyc pliku <outfile>");
        exit(EXIT_FAILURE); 
    }

    taskfile_filename = malloc(strlen(argv[1]) + 1);
    strcpy(taskfile_filename, argv[1]);

    taskfile_filename[strlen(argv[1])] = '\0';




    
    signal(SIGCHLD, SIG_IGN);

    //Main minicron process
    list = list_create();

    read_file(taskfile_fd);
    close(taskfile_fd);  

    list_sort(list,&task_compare);

    signal(SIGUSR1, signal_handler_reload_tasks);
    signal(SIGUSR2, signal_handler_log_tasks);
    signal(SIGINT, signal_handler_terminate_all);

    deamon_loop();


}

//Minicron main loop
void deamon_loop()
{
    while(list->size > 0)
    {
        if(irruption_code == LOG_SIG)
        {
            print_tasks(list);
            irruption_code = NO_SIG;
        }
        else if(irruption_code == RELOAD_SIG)
        {
            reload_list();
            irruption_code = NO_SIG;
            continue;
        }
        else if(irruption_code == TERMINATE_SIG)
        {
            terminate_all();
            irruption_code = NO_SIG;
        }

        Task* task = list_peek(list);
        int sleepTime = min_dif(task);
        //If process is at next day. add 24 hours
        if(sleepTime < 0)
            sleepTime = sleepTime + 24*60;
        
        //Start task if is time or it was very close ago
        if(sleepTime == 0 || sleepTime == -1) 
        {
            start_task(task);
            list_pop(list);
        }
        //Go sleep
        else if(sleepTime > 0)
        {
            struct timespec request,remaining;
            request.tv_nsec = 0;
            request.tv_sec = sleepTime*60-sec_now();

            while(nanosleep(&request,&remaining) == -1)
            {
                //Printing tasks dont change anything
                if(irruption_code == LOG_SIG)
                {
                    print_tasks(list);
                    irruption_code = NO_SIG;
                }
    
                //continue loop, handle irruption code at start of loop
                if(irruption_code == TERMINATE_SIG)
                    break;
                if(irruption_code == RELOAD_SIG)
                    break;

                sleepTime = min_dif(task);
                if(sleepTime < 0)
                    sleepTime = sleepTime + 24*60*60;

                request.tv_sec = sleepTime*60-sec_now();
            
            }

            if(irruption_code == RELOAD_SIG)
                continue;
            if(irruption_code == TERMINATE_SIG)
                continue;

            start_task(task);
            list_pop(list);

        }

    
    }
    //All tasks done
    list_destroy(list);
    list = NULL;
}


//Starting new task as [process/processes with pipeline]
void start_task(Task* task)
{
    char* text = malloc(sizeof(char)*(strlen(task->command)+4));
    text[0] = '[';
    for(int i=1; i < (int)strlen(task->command)+1;i++)
    {
        text[i] = task->command[i-1];
    }
    text[strlen(task->command)+1] = ']';
    text[strlen(task->command)+2] = '\n';
    text[strlen(task->command)+3] = '\0';
    ssize_t bytes_written = write(outfile_fd, text, strlen(text));
    
    free(text);

    List* jobs_list = get_jobs(task->command);



    int jobs_count = jobs_list->size;
    int index = 0;
    int mode = task->mode;


    if (bytes_written == -1) {
        perror("Blad przy pisaniu nazwy processu w pliku <output>\n");
        close(outfile_fd);
        exit(EXIT_FAILURE);
    }

    //Only one Process so [] pipe
    if(jobs_list->size == 1)
    {
        List* prop = list_pop(jobs_list);
        create_one_process(prop,mode);
        free(task->command);
        task->command = NULL;
        free(task);
        task = NULL;
        list_free_elements(prop);
        list_destroy(prop);
        list_destroy(jobs_list);
        return;
    }

    //More than one process in |
    int fd0 = -1;
    while(index < jobs_count)
    {
        //First Process so [OUT] pipe
        if(index == 0)
        {
            List* prop = list_pop(jobs_list);
            fd0 = create_first_process(prop,mode);
            list_free_elements(prop);
            list_destroy(prop);
        }
        //Last Process so [IN] pipe
        else if(index == jobs_count - 1)
        {
            if(fd0 < 0) 
            {
                perror("pipe[fd0]");
                exit(EXIT_FAILURE);
            }
            List* prop = list_pop(jobs_list);
            create_last_process(prop,fd0,mode);
            list_free_elements(prop);
            list_destroy(prop);
            free(task->command);
            task->command = NULL;
            free(task);
            task = NULL;
            list_destroy(jobs_list);
            return;
        }
        //Process in middle so [IN/OUT] pipe
        else
        {
            if(fd0 < 0) 
            {
                perror("pipe[fd0]");
                exit(EXIT_FAILURE);
            }
            List* prop = list_pop(jobs_list);
            fd0 = create_middle_process(prop,fd0,mode);
            list_free_elements(prop);
            list_destroy(prop);
        }
        index++;

    }
}

//Setting mode to process
void output_mode(int mode,bool lastProcess)
{

    if(mode == 0)
    {
        if(!lastProcess)
            return;

        if(dup2(outfile_fd,STDOUT_FILENO) < 0)
        {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
    }
    else if(mode == 1)
    {
        if(dup2(outfile_fd,STDERR_FILENO) < 0)
        {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
    }
    else if(mode == 2)
    {
        if(dup2(outfile_fd,STDERR_FILENO) < 0)
        {
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        if(!lastProcess)
            return;
        if(dup2(outfile_fd,STDOUT_FILENO) < 0)
        {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
    }
}

//Reloading a list
void reload_list()
{
    while(list->size > 0)
    {
        Task* task = list_pop(list);
        
        if(task != NULL)
        {
            if(task->command != NULL)
                free(task->command);
            free(task);
        }

    }
    list_destroy(list);
    list = list_create();
    taskfile_fd = open(taskfile_filename,O_RDONLY);
    if(taskfile_fd == -1)
    {
        perror("minicron: nie mozna otworzyc pliku <taskfile>");
        exit(EXIT_FAILURE); 
    }
    read_file(taskfile_fd);
    close(taskfile_fd);  

    list_sort(list,&task_compare);
}

//Reading file and adding tasks to global list
void read_file(int fd)
{

    FILE *fp = fdopen(fd, "r");

    if(fp == NULL) {
           perror("minicron: nie mozna otworzyc pliku <taskfile>");
           exit(EXIT_FAILURE);
    }

    char *line = NULL;
    size_t len = 0;

    while(getline(&line, &len, fp) != -1) {
        Task* task = create_task(line);
        if(task != NULL)
            list_add(list,task);

    }

    fclose(fp);
    free(line);     
}
/////////////////////////////////////

//Clear all data and close process
void terminate_all()
{
    
    while(list->size > 0)
    {
        Task* t = (Task*)list_pop(list);
        if(t != NULL)
        {
            if(t->command != NULL)
                free(t->command);
            free(t);
        }
        
    }

    list_destroy(list);

    set_default_signals();

    int pid;
    int status;
    while ((pid = waitpid(-1, &status, 0)) > 0);

    exit(EXIT_SUCCESS);

}
//Settings signals to they default behaviour
void set_default_signals()
{
    signal(SIGCHLD, SIG_DFL);
    signal(SIGINT, SIG_IGN);
    signal(SIGUSR1, SIG_IGN);
    signal(SIGUSR2, SIG_IGN);
}
//Calculate time difference functions
int min_dif(Task* task)
{
    time_t now;
    now = time(NULL);
    struct tm* time_info;
    time_info = localtime(&now);

    int now_min = time_info->tm_hour*60 + time_info->tm_min;
    int task_min = task->hour*60 + task->minutes;

    return task_min - now_min;
}
int sec_now()
{
    time_t now;
    now = time(NULL);
    struct tm* time_info;
    time_info = localtime(&now);
    return time_info->tm_sec;
}
/////////////////////////////////////

// Signals Handlers
void signal_handler_reload_tasks() {
    irruption_code = RELOAD_SIG;
}
void signal_handler_log_tasks() {
    irruption_code = LOG_SIG;

}
void signal_handler_terminate_all() {
    irruption_code = TERMINATE_SIG;

}

/////////////////////////////////////

//Creating [First/I-th/Last] Process in pipeline
int create_first_process(List* prop,int mode)
{
    
    int fd[2];

    if(pipe(fd) < 0)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    int uid = fork();
    if(uid < 0)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    char* tmp_prop[prop->size+1];
    for (int i = 0; i < prop->size; i++)
    {
        tmp_prop[i] = malloc(sizeof(char)*(strlen((char*)prop->array[i])+1));
        strcpy(tmp_prop[i], (char*)prop->array[i]);
    }
    tmp_prop[prop->size] = NULL;

    if(uid > 0)
    {
        for (int i = 0; i < prop->size; i++)
            free(tmp_prop[i]);

        if(close(fd[1]) < 0)
        {
            perror("close");
            exit(EXIT_FAILURE);
        }

        return fd[0];
    }

    set_default_signals();

    create_observer_process(tmp_prop[0]);

    output_mode(mode,false);

    if(dup2(fd[1],STDOUT_FILENO) < 0)
    {
        perror("dup2");
        exit(EXIT_FAILURE);
    }
    if(close(fd[0]) < 0)
    {
        perror("close");
        exit(EXIT_FAILURE);
    }
    if(close(fd[1]) < 0)
    {
        perror("close");
        exit(EXIT_FAILURE);
    }

    syslog(LOG_USER, "Odpalono [%s] pid[%d] ",tmp_prop[0],getpid());
    if(execvp(tmp_prop[0],tmp_prop) < 0)
    {
        perror("execlp");
        exit(EXIT_FAILURE);
    }

    //Nor parent nor child should execute this line
    exit(EXIT_FAILURE);
}
void create_last_process(List* prop,int fd0,int mode)
{
    int uid = fork();
    if(uid < 0)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    char* tmp_prop[prop->size+1];

    for (int i = 0; i < prop->size; i++)
    {
        tmp_prop[i] = malloc(sizeof(char)*(strlen((char*)prop->array[i])+1));
        strcpy(tmp_prop[i], (char*)prop->array[i]);
    }
    tmp_prop[prop->size] = NULL;

    if(uid > 0)
    {
        for (int i = 0; i < prop->size; i++)
            free(tmp_prop[i]);
        
        if(close(fd0) < 0)
        {
            perror("close");
            exit(EXIT_FAILURE);
        }

        return;
    }

    set_default_signals();

    create_observer_process(tmp_prop[0]);

    output_mode(mode,true);

    if(dup2(fd0,STDIN_FILENO) < 0)
    {
        perror("dup2");
        exit(EXIT_FAILURE);
    }
    if(close(fd0) < 0)
    {
        perror("close");
        exit(EXIT_FAILURE);
    }

    syslog(LOG_USER, "Odpalono [%s] pid[%d]",tmp_prop[0],getpid());
    

    if(execvp(tmp_prop[0],tmp_prop) < 0)
    {
        perror("execlp");
        exit(EXIT_FAILURE);
    }

    //Nor parent nor child should execute this line
    exit(EXIT_FAILURE);
}
int create_middle_process(List* prop,int fd0,int mode)
{
    int fd[2];

    if(pipe(fd) < 0)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    int uid = fork();
    if(uid < 0)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    char* tmp_prop[prop->size+1];

    for (int i = 0; i < prop->size; i++)
    {
        tmp_prop[i] = malloc(sizeof(char)*(strlen((char*)prop->array[i])+1));
        strcpy(tmp_prop[i], (char*)prop->array[i]);
    }
    tmp_prop[prop->size] = NULL;

    if(uid > 0)
    {
        for (int i = 0; i < prop->size; i++)
            free(tmp_prop[i]);

        if(close(fd[1]) < 0)
        {
            perror("close");
            exit(EXIT_FAILURE);
        }
        if(close(fd0) < 0)
        {
            perror("close");
            exit(EXIT_FAILURE);
        }

        return fd[0];
    }

    set_default_signals();

    create_observer_process(tmp_prop[0]);
    
    output_mode(mode,false);

    if(dup2(fd0,STDIN_FILENO) < 0)
    {
        perror("dup2");
        exit(EXIT_FAILURE);
    }
    if(dup2(fd[1],STDOUT_FILENO) < 0)
    {
        perror("dup2");
        exit(EXIT_FAILURE);
    }
    if(close(fd[0]) < 0)
    {
        perror("close");
        exit(EXIT_FAILURE);
    }
    if(close(fd[1]) < 0)
    {
        perror("close");
        exit(EXIT_FAILURE);
    }
    if(close(fd0) < 0)
    {
        perror("close");
        exit(EXIT_FAILURE);
    }

    syslog(LOG_USER, "Odpalono [%s] pid[%d]",tmp_prop[0],getpid());

    if(execvp(tmp_prop[0],tmp_prop) < 0)
    {
        perror("execlp");
        exit(EXIT_FAILURE);
    }

    //Nor parent nor child should execute this line
    exit(EXIT_FAILURE);
}

//Create single process without pipeline
void create_one_process(List* prop,int mode)
{


    int uid = fork();
    if(uid < 0)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    char* tmp_prop[prop->size+1];
    for (int i = 0; i < prop->size; i++)
    {
        tmp_prop[i] = malloc(sizeof(char)*(strlen((char*)prop->array[i])+1));
        strcpy(tmp_prop[i], (char*)prop->array[i]);
    }
    tmp_prop[prop->size] = NULL;

    if(uid > 0)
    {
        for (int i = 0; i < prop->size; i++)
            free(tmp_prop[i]);

        return;
    }
    set_default_signals();

    create_observer_process(tmp_prop[0]);

    output_mode(mode,true);

    syslog(LOG_USER, "Odpalono [%s] pid [%d] ",tmp_prop[0],getpid());


    if(execvp(tmp_prop[0],tmp_prop) < 0)
    {
        perror("execlp");
        exit(EXIT_FAILURE);
    }
}

//Process that is just waiting for his child to end and write his return code
void create_observer_process(char* proc_name)
{
    int pid = fork();
    if(pid == 0)
    {
        return;
    }
    int status;
    int w;
    do 
    {

        w = waitpid(pid, &status, WUNTRACED);
        if (w == -1) {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }

        if (WIFEXITED(status)) 
        {
            syslog(LOG_USER,"Process[%s] pid[%d]zakonczony, status=%d\n",proc_name,pid, WEXITSTATUS(status));
        } 
        else if (WIFSIGNALED(status)) 
        {
            syslog(LOG_USER,"Process[%s] pid[%d] killed z przez [%d]\n",proc_name,pid, WTERMSIG(status));
        } 
        else if (WIFSTOPPED(status)) 
        {
            syslog(LOG_USER,"Process[%s] pid[%d] zatrzymany przez [%d]\n",proc_name,pid, WSTOPSIG(status));
        }
        else if (WIFCONTINUED(status)) 
        {
            syslog(LOG_USER,"Process[%s] pid[%d] kontynuowany\n",proc_name,pid);
        }
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    exit(EXIT_SUCCESS);
}
//Deamonize process
void daemonize()
{
        /* Our process ID and Session ID */
        pid_t pid, sid;
        
        /* Fork off the parent process */
        pid = fork();
        if (pid < 0) {
                exit(EXIT_FAILURE);
        }
        /* If we got a good PID, then
           we can exit the parent process. */
        if (pid > 0) {
                exit(EXIT_SUCCESS);
        }
           
        /* Create a new SID for the child process */
        sid = setsid();
        if (sid < 0) {
                exit(EXIT_FAILURE);
        }

}
