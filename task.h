#include <syslog.h>
#include "list.h"
#include <time.h>

typedef struct 
{
    int hour;
    int minutes;
    char* command;
    int mode;
} Task;

Task* create_task(char*);
int task_compare(void*, void*);
List* get_jobs(char*);
void print_tasks(List*);

