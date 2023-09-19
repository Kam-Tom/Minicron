#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "task.h"

List* get_parameters(char*);

List* get_parameters(char* line)
{
    List* list = list_create();
    char *token;
    token = strtok(line, " ");

    while(token != NULL)
    {
        char* param = malloc(sizeof(char) * (strlen(token)+1));
        strcpy(param,token);

        list_add(list,param);

        token = strtok(NULL, " ");
    }
    return list;
}
List* get_jobs(char* line)
{

    List* list = list_create();

    List* tmp_list = list_create();

    char *token;
    token = strtok(line, "|");
    while(token != NULL)
    {
        char* command = malloc(sizeof(char) * (strlen(token)+1));
        strcpy(command,token);

        list_add(tmp_list,command);
        token = strtok(NULL, "|");
    }
    while(tmp_list->size > 0)
    {
        char* command = (char*)list_pop(tmp_list);

        List* param_list = get_parameters(command);
        
        list_add(list,param_list);

        free(command);
    }

    list_destroy(tmp_list);


    return list;

}

void print_tasks(List* list)
{
    for(int i=0;i<list->size;i++)
    {
        Task* task = (Task*)list_peek_index(list,i);
        syslog(LOG_USER,"Proces w kolejce %d:%d:%s:%d ",task->hour,task->minutes,task->command,task->mode);
    }

}

Task* create_task(char* line)
{
    char *token;
    Task* task = malloc(sizeof(Task));

    token = strtok(line, ":");
    if (token == NULL) 
    {
        fprintf(stderr,"minicron: niepoprawna komenda, plik <taskile> powinien miec format <hour>:<minutes>:<command>:<mode> a otrzymano %s\n",line);
        free(task);
        return NULL;
    }
    task->hour = atoi(token);
    if (task->hour < 0 || task->hour > 23) 
    {
        fprintf(stderr,"minicron: niepoprawna godzina w pliku <taskile> godzina powinna byc w przedziale 0-23, a otrzymano %s\n",line);
        free(task);
        return NULL;
    }
    token = strtok(NULL, ":");
    if (token == NULL) 
    {
        fprintf(stderr,"minicron: niepoprawna komenda, plik <taskile> powinien miec format <hour>:<minutes>:<command>:<mode> a otrzymano %s\n",line);
        free(task);
        return NULL;
    }
    task->minutes = atoi(token);
    if (task->hour < 0 || task->hour > 23) 
    {
        fprintf(stderr,"minicron: niepoprawna minuta w pliku <taskile> minuta powinna byc w przedziale 0-59, a otrzymano %s\n",line);
        free(task);
        return NULL;
    }
    token = strtok(NULL, ":");  
    if (token == NULL) 
    {
        fprintf(stderr,"minicron: niepoprawna komenda, plik <taskile> powinien miec format <hour>:<minutes>:<command>:<mode> a otrzymano %s\n",line);
        free(task);
        return NULL;
    }
    task->command = malloc(sizeof(char) * (strlen(token)+1));
    strcpy(task->command,token);
    token = strtok(NULL, ":");
    if (token == NULL) 
    {
        fprintf(stderr,"minicron: niepoprawna komenda, plik <taskile> powinien miec format <hour>:<minutes>:<command>:<mode> a otrzymano %s\n",line);
        free(task);
        free(task->command);
        return NULL;
    }
    task->mode = atoi(token);
    if (task->hour < 0 || task->hour > 23) 
    {
        fprintf(stderr,"minicron: niepoprawny tryb w pliku <taskile> tryb powinien byc w przedziale 0-3, a otrzymano %s\n",line);
        free(task);
        free(task->command);
        return NULL;
    }

    return task;
}

int task_compare(void* a, void* b) {

    time_t now;
    now = time(NULL);
    struct tm* time_info;
    time_info = localtime(&now);

    Task* task_a = (Task*)a;
    Task* task_b = (Task*)b;

    int now_min = time_info->tm_hour*60 + time_info->tm_min;
    int task_a_min = task_a->hour*60 + task_a->minutes;
    int task_b_min = task_b->hour*60 + task_b->minutes;

    if(task_a_min < now_min)
        task_a_min += 24*60;
    if(task_b_min < now_min)
        task_b_min += 24*60;


    if (task_a_min < task_b_min) 
    {
        return 1;
    } 
    else if (task_b_min > task_a_min) 
    {
        return -1;
    } 
    else 
    {
       return 0;
    }
}