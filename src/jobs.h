//
// Created by nkinder on 8/16/26.
//

#pragma once
#include <sys/types.h>
#include <signal.h>


#define MAX_JOBS 10

typedef struct Job {
    pid_t pid;
    int job_number;
    const char* cmdline;
} Job;

extern Job* jobs[MAX_JOBS];
extern int job_count;

extern volatile sig_atomic_t child_exited_flag;

Job* init_job(pid_t pid, int job_number, const char* cmdline[]);

void cleanup_job(Job* job);

void print_job_imm(Job* job);

void print_job(Job* job);

void print_jobs();

void return_job_number(int job_number);

Job* get_job(pid_t pid);

int append_job(pid_t job, const char* cmdline[]);

int get_next_job_number();

void check_background_jobs();

void sigchld_handler(int signum);

int remove_job(pid_t pid);

void print_job_exit(pid_t pid, int job_number);