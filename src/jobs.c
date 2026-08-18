//
// Created by nkinder on 8/16/26.
//
#include "jobs.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

volatile sig_atomic_t child_exited_flag = 0;

void
print_job(Job job) {
    printf("[%d]    %d\n", job.job_number, job.pid);
    fflush(stdout);
}


Job jobs[MAX_JOBS] = {0};
int job_count = 0;

bool job_numbers[MAX_JOBS] = {false};

void
return_job_number(int job_number) {
    job_numbers[job_number - 1] = false;
}

int get_next_job_number() {
    if (job_count == MAX_JOBS) {
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



int append_job(pid_t job) {
  if (job_count == MAX_JOBS) {
      return -1;
  }
    int ret = job_count;
    jobs[job_count++] = (Job){job, get_next_job_number()} ;

    print_job(jobs[ret]);

    return ret;
}
Job* get_job(pid_t pid) {
    for (int i = 0; i < job_count; ++i) {
        if (jobs[i].pid == pid) {
            return &jobs[i];
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
        Job* job = get_job(pid);
        if (job) {
            int job_number = job->job_number;
            remove_job(pid);
            return_job_number(job_number);
            print_job_exit(pid, job_number);

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
        if (jobs[i].pid == pid) {
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

