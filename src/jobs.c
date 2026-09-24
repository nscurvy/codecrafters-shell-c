//
// Created by nkinder on 8/16/26.
//
#include "jobs.h"

#include "common.h"
#include "parser.h"
#include "stringbuilder.h"
#include <math.h>

#include <readline/readline.h>

typedef struct JobNode {
    Job*            job;
    struct JobNode* next;
} JobNode;

typedef struct JobList {
    size_t   size;
    JobNode* head;
} JobList;

JobList*
jl_new() {
    JobList* list = malloc(sizeof(JobList));
    list->size    = 0;
    list->head    = nullptr;
    return list;
}

JobNode*
jn_new(Job* job) {
    JobNode* node = malloc(sizeof(JobNode));
    node->job     = job;
    node->next    = nullptr;
    return node;
}


void
jl_append(JobList* list, Job* job) {
    JobNode* node = jn_new(job);
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

void
jn_delete(JobNode* node) {
    Job* job = node->job;

    job_delete(job);
    free(node);
}

void
jl_remove(JobList* list, pid_t pid) {
    JobNode* iter = list->head;
    JobNode* prev = nullptr;
    while (iter != nullptr) {
        if (iter->job->pid == pid) {
            if (prev == nullptr) {
                list->head = iter->next;
            } else {
                prev->next = iter->next;
            }
            jn_delete(iter);
            return;
        }
        prev = iter;
        iter = iter->next;
    }
}

JobList* job_list = nullptr;

volatile sig_atomic_t child_exited_flag = 0;

void
optostr(StringBuilder* sb, AndOrOp op) {
    switch (op) {
    case AND_AND:
        sb_appends(sb, "&&");
        break;
    case AND_OR:
        sb_appends(sb, "||");
        break;
    default:
        break;
    }
}

const char*
join_andor(AndOr* and_or) {
    AndOrElement*  iter = and_or->head;
    StringBuilder* sb   = sb_new();

    size_t totalsize = 0;
    size_t i         = 0;
    while (iter != nullptr) {
        optostr(sb, iter->op);
        if (i++ > 0) {
            sb_appendc(sb, ' ');
        }
        join_pipeline(iter->pipeline, sb);
        if (iter->next != nullptr) {
            sb_appendc(sb, ' ');
        }
        iter = iter->next;
    }
    const char* result = sb_takestring(sb);
    sb_delete(sb);

    return result;
}

void
join_pipeline(Pipeline* pipeline, StringBuilder* sb) {
    PipelineElement* iter = pipeline->head;

    while (iter != nullptr) {
        join_command(iter->command, sb);
        if (iter->next != nullptr) {
            sb_appends(sb, " | ");
        }

        iter = iter->next;
    }
}


void
join_args(const char** args, StringBuilder* sb) {
    const char** aiter = args;
    while (*aiter != nullptr) {
        sb_appends(sb, *aiter);
        if (*(aiter + 1) != nullptr) {
            sb_appendc(sb, ' ');
        }
        ++aiter;
    }
}

void
join_redirect(Redirect* redirect, StringBuilder* sb) {
    if (redirect->fd > 2) {
        sb_appendl(sb, redirect->fd);
    }
    if (redirect->mode == REDIR_IN) {
        sb_appendc(sb, '<');
    } else if (redirect->mode == REDIR_APPEND) {
        if (redirect->fd == 2) {
            sb_appendl(sb, redirect->fd);
        }
        sb_appends(sb, ">>");
    } else if (redirect->mode == REDIR_OUT) {
        if (redirect->fd == 2) {
            sb_appendl(sb, redirect->fd);
        }
        sb_appendc(sb, '>');
    } else if (redirect->mode == REDIR_HEREDOC) {
        sb_appends(sb, "<<");
    }
    sb_appendc(sb, ' ');
    sb_appends(sb, redirect->target);
}

void
join_command(Command* command, StringBuilder* sb) {
    Assignment* iter = command->assignment_list;
    while (iter != nullptr) {
        sb_appends(sb, iter->value);
        sb_appendc(sb, ' ');
        iter = iter->next;
    }
    join_args((const char**) command->argv, sb);
    if (command->nredirs > 0) {
        sb_appendc(sb, ' ');
    }
    for (int i = 0; i < command->nredirs; ++i) {
        join_redirect(&command->redirs[i], sb);
        if ((size_t) i < command->nredirs - 1) {
            sb_appendc(sb, ' ');
        }
    }
}


Job*
get_job_by_pid(JobList* list, pid_t pid) {
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
job_new(pid_t pid, int job_number, const char* cmdline) {
    Job* new_job = (Job*) malloc(sizeof(Job));
    if (!new_job) {
        return nullptr;
    }
    new_job->pid        = pid;
    new_job->cmdline    = strdup(cmdline);
    new_job->job_number = job_number;

    return new_job;
}

void
job_delete(Job* job) {
    free(job->cmdline);
    free(job);
}

void
job_print_imm(Job* job) {
    printf("[%d] %d\n", job->job_number, job->pid);
    fflush(stdout);
}

void
print_job_with_status(Job* job, const char* status) {
    const char* status_symbol = " ";
    JobNode*    iter          = job_list->head;
    JobNode*    prev          = nullptr;
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
    // if (jobs[job_count - 1]->pid == job->pid) {
    //     status_symbol = "+";
    // } else if (job_count > 1 && jobs[job_count - 2]->pid == job->pid) {
    //     status_symbol = "-";
    // }
    const char* cmdline = job->cmdline;
    int         job_num = job->job_number;

    printf("[%d]%s  %-20s %s\n", job_num, status_symbol, status, cmdline);
}


int
check_and_print_job(Job* job) {
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
    if (job_list == nullptr) {
        job_list = jl_new();
    }
    JobNode* iter = job_list->head;
    JobNode* prev = nullptr;
    while (iter != nullptr) {
        int res = check_and_print_job(iter->job);
        if (res) {
            jl_remove(job_list, iter->job->pid);
            iter = (prev == nullptr) ? job_list->head : prev->next;
        } else {
            prev = iter;
            iter = iter->next;
        }
    }

    check_background_jobs();
}


int
get_next_job_number() {
    if (job_list == nullptr) {
        job_list = jl_new();
    }

    int  candidate = 1;
    bool found;
    do {
        found      = false;
        JobNode* i = job_list->head;
        while (i != nullptr) {
            if (i->job->job_number == candidate) {
                found = true;
                candidate++;
                break;
            }
            i = i->next;
        }
    } while (found);

    return candidate;

    return -1;
}


int
register_job(pid_t job, const char* cmdline) {
    if (!job_list) {
        job_list = jl_new();
    }
    int  ret     = (int) job_list->size;
    int  job_num = get_next_job_number();
    Job* new_job = job_new(job, job_num, cmdline);
    jl_append(job_list, new_job);

    job_print_imm(new_job);

    return ret;
}


void
report_and_reap_jobs() {
    // Save the current spot in readline and then get ready to display job info
    int   saved_point = -1;
    char* saved_line  = nullptr;
    bool  displayed   = false;


    rl_copy_text(0, rl_end);
    int   status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        Job* job = get_job_by_pid(job_list, pid);
        if (job) {
            if (!displayed) {
                saved_point = rl_point;
                saved_line  = rl_copy_text(0, rl_end);
                rl_save_prompt();
                rl_replace_line("", 0);
                rl_redisplay();
                printf("\n");
                displayed = true;
            }

            print_job_with_status(job, "Done");
            jl_remove(job_list, pid);
        }
    }

    if (displayed) {
        // Now restore everything to how it was.
        rl_restore_prompt();
        rl_replace_line(saved_line, 0);
        rl_point = saved_point;
        rl_forced_update_display();
        fflush(stdout);
        // rl_redisplay();
        free(saved_line);
    }
}

int
check_background_jobs() {
    if (child_exited_flag) {
        child_exited_flag = 0;
        report_and_reap_jobs();
    }
    return 0;
}

void
sigchld_handler(int signum) {
    child_exited_flag = 1;
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

    char* arg       = buffer;
    int   arg_count = 0;
    while (arg < buffer + bytes_read && *arg != '\0') {
        printf("%s ", arg);
        arg += strlen(arg) + 1;
    }

    printf("\n");
}
