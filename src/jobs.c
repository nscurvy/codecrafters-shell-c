//
// Created by nkinder on 8/16/26.
//
#include "jobs.h"

#include "common.h"

volatile sig_atomic_t child_exited_flag = 0;


typedef struct JobNode {
    Job* job;
    struct JobNode* next;
} JobNode;

typedef struct JobList {
    size_t size;
    JobNode* head;
} JobList;

JobList* init_job_list() {
    JobList* list = malloc(sizeof(JobList));
    list->size = 0;
    list->head = nullptr;
    return list;
}

JobNode* init_job_node(Job* job) {
    JobNode* node = malloc(sizeof(JobNode));
    node->job = job;
    node->next = nullptr;
    return node;
}

JobList* job_list = nullptr;

void append_job_list(JobList* list, Job* job) {
    JobNode* node = init_job_node(job);
    if (list->head == nullptr) {
        list->head = node;
        list->size = 1;
        return;
    }
    JobNode* iter = list->head;
    while (iter->next != nullptr) {
        iter = iter->next;
    }
    iter->next = node;
    list->size++;
}

void cleanup_job_node(JobNode* node) {
    Job* job = node->job;

    cleanup_job(job);
    free(node);
}

void remove_job_node(JobList* list, pid_t pid) {
    JobNode* iter = list->head;
    JobNode* prev = nullptr;
    while (iter != nullptr) {
        if (iter->job->pid == pid) {
            if (prev == nullptr) {
                list->head = iter->next;
            } else {
                prev->next = iter->next;
            }
            return_job_number(iter->job->job_number);
            cleanup_job_node(iter);
            return;
        }
        prev = iter;
        iter = iter->next;
    }
}

Job* get_job_by_pid(JobList* list, pid_t pid) {
    JobNode* iter = list->head;
    while (iter != nullptr) {
        if (iter->job->pid == pid) {
            return iter->job;
        }
        iter = iter->next;
    }
    return nullptr;
}

Job*
init_job(pid_t pid, int job_number, const char** cmdline) {
    Job* new_job = (Job*)malloc(sizeof(Job));
    if (!new_job) {
        return nullptr;
    }
    const char** iter = cmdline;
    size_t required_length = 0;
    while (*iter != nullptr) {
        required_length += strlen(*iter);
        required_length += 1;
        ++iter;
    }

    char* cmd = (char*)calloc(required_length, sizeof(char));
    if (!cmd) {
        free(new_job);
        return nullptr;
    }
    iter = cmdline;
    char* pos = cmd;
    while (*iter != nullptr) {
        strcat(pos, *iter);
        pos += strlen(*iter);
        if (*(iter + 1) == NULL) {
            *pos = '\0';
        } else {
            *pos = ' ';
            ++pos;
        }
        ++iter;
    }
    new_job->pid = pid;
    new_job->cmdline = cmd;
    new_job->job_number = job_number;

    return new_job;
}

void
cleanup_job(Job* job) {
    free(job->cmdline);
    free(job);

}

void
print_job_imm(Job* job) {
    printf("[%d] %d\n", job->job_number, job->pid);
    fflush(stdout);
}

void print_job_with_status(Job* job, const char* status) {
    const char* status_symbol = " ";
    JobNode* iter = job_list->head;
    JobNode* prev = nullptr;
    while (iter != nullptr) {
        if (iter->next == nullptr && job->pid == iter->job->pid) {
            status_symbol = "+";
            break;
        }
        if (iter->next == nullptr && job->pid == prev->job->pid) {
            status_symbol = "-";
            break;
        }
        prev = iter;
        iter = iter->next;
    }
    //if (jobs[job_count - 1]->pid == job->pid) {
    //    status_symbol = "+";
    //} else if (job_count > 1 && jobs[job_count - 2]->pid == job->pid) {
    //    status_symbol = "-";
    //}
    const char* cmdline = job->cmdline;
    int job_num = job->job_number;

    printf("[%d]%s  %-20s %s\n", job_num,status_symbol, status, cmdline);
    fflush(stdout);

}

void print_job(Job* job) {
    const char* status_symbol = " ";
    if (jobs[job_count - 1]->pid == job->pid) {
        status_symbol = "+";
    } else if (job_count > 1 && jobs[job_count - 2]->pid == job->pid) {
        status_symbol = "-";
    }
    const char* cmdline = job->cmdline;
    int job_num = job->job_number;

    printf("[%d]%s  %-20s %s\n", job_num,status_symbol, "Running", cmdline);
    fflush(stdout);
}

int check_and_print_job(Job* job) {
    if ((waitpid(job->pid, nullptr, WNOHANG) == job->pid)) {
        print_job_with_status(job, "Done");
        return 1;
    } else {
        print_job_with_status(job, "Running");
        return 0;
    }
}

void
print_jobs() {
    JobNode* iter = job_list->head;
    JobNode* prev = nullptr;
    while (iter != nullptr) {
        int res = check_and_print_job(iter->job);
        if (res) {
            remove_job_node(job_list, iter->job->pid);
            iter = (prev == nullptr) ? job_list->head : prev->next;
        } else {
            prev = iter;
            iter = iter->next;
        }
    }

    check_background_jobs();
}


Job* jobs[MAX_JOBS] = {0};
int job_count = 0;

bool job_numbers[MAX_JOBS] = {false};

void
return_job_number(int job_number) {
    job_numbers[job_number - 1] = false;
}

int get_next_job_number() {
    if (job_list == nullptr) {
        job_list = init_job_list();
    }
    if (job_list->size == MAX_JOBS) {
        return -1;
    }

    for (int i = 0; i < MAX_JOBS; ++i) {
        if (!job_numbers[i]) {
            job_numbers[i] = true;
            return i + 1;
        }
    }
    return -1;
}



int append_job(pid_t job, const char** cmdline) {
  if (job_count == MAX_JOBS) {
      return -1;
  }
    int ret = job_count;
    int job_num = get_next_job_number();
    Job* new_job = init_job(job, job_num, cmdline);
    if (job_list == nullptr) {
        job_list = init_job_list();
    }
    append_job_list(job_list, new_job);
    //jobs[job_count++] = new_job;

    print_job_imm(new_job);

    return ret;
}

Job* get_job(pid_t pid) {
    for (int i = 0; i < job_count; ++i) {
        if (jobs[i]->pid == pid) {
            return jobs[i];
        }
    }
    return nullptr;
}

void
check_background_jobs() {
    if (!child_exited_flag) {
        return;
    }
    child_exited_flag = 0;

    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        Job* job = get_job_by_pid(job_list, pid);
        if (job) {
            int job_number = job->job_number;

            //print_job_with_status(job, "Done");
            remove_job_node(job_list, pid);
            return_job_number(job_number);

        }
    }
}

void
sigchld_handler(int signum) {
    child_exited_flag = 1;
}

int
remove_job(pid_t pid) {
    int i = 0;
    for (i = 0; i < job_count; ++i) {
        if (jobs[i]->pid == pid) {
            Job* old_job = jobs[i];
            cleanup_job(old_job);
            jobs[i] = nullptr;
            break;
        }
    }
    for (int j = i; j < job_count - 1; ++j) {
        jobs[j] = jobs[j + 1];
    }
    --job_count;
    return i;
}

void
print_job_exit(pid_t pid, int job_number) {
    char path[64];
    char buffer[4096];

    snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);

    printf("[%d]    %d done\t", job_number, pid);
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        return;
    }

    ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);

    if (bytes_read <= 0) {
        printf("\n");
        return;
    }

    buffer[bytes_read] = '\0';

    char* arg = buffer;
    int arg_count = 0;
    while (arg < buffer + bytes_read && *arg != '\0') {
        printf("%s ", arg);
        arg += strlen(arg) + 1;
    }

    printf("\n");
}

