//
// Created by nkinder on 8/16/26.
//

#pragma once
#include "nullability.h"
#include <signal.h>


#define MAX_JOBS 10

typedef struct Job {
    pid_t       pid;
    int         job_number;
    const char* cmdline;
} Job;

ASSUME_NONNULL_BEGIN

extern Job* jobs[MAX_JOBS];
extern int  job_count;

extern volatile sig_atomic_t child_exited_flag;

Job*
init_job(pid_t pid, int job_number, const char** NONNULL cmdline) GCC_NONNULL(3);

void
cleanup_job(Job* NONNULL job) GCC_NONNULL(1);

void
print_job_imm(Job* NONNULL job) GCC_NONNULL(1);

void
print_job(Job* NONNULL job) GCC_NONNULL(1);

void
print_jobs();

void
return_job_number(int job_number);

Job* NULLABLE
get_job(pid_t pid);

int
append_job(pid_t job, const char* NONNULL cmdline[]);

int
get_next_job_number();

int
check_background_jobs();

void
sigchld_handler(int signum);

int
remove_job(pid_t pid);

void
print_job_exit(pid_t pid, int job_number);

ASSUME_NONNULL_END
