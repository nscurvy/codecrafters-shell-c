//
// Created by nkinder on 8/16/26.
//

#pragma once
#include "nullability.h"
#include <signal.h>


struct AndOr;
struct Pipeline;
struct Command;
struct Redirect;
struct StringBuilder;

typedef enum JobStatus { JOB_RUNNING, JOB_STOPPED, JOB_DONE } JobStatus;

typedef struct Job {
    pid_t       pid;
    int         job_number;
    const char* cmdline;
    JobStatus   status;
} Job;

ASSUME_NONNULL_BEGIN


extern volatile sig_atomic_t child_exited_flag;

/**
 * Join a AndOr node into a string representing a normalized view of the cmdline text
 * that would execute that AndOr.
 *
 * @param and_or The AndOr to join
 * @return A string representing the AndOr.
 */
const char*
join_andor(struct AndOr* and_or);

/**
 * Join a Pipeline node into a string representing a normalized view of the cmdline text
 * that would execute that pipeline.
 *
 * @param pipeline The pipeline to join
 */
void
join_pipeline(struct Pipeline* pipeline, struct StringBuilder* sb);

/**
 * Join a Command node into a string representing a normalized view of the cmdline text
 * that would execute that command.
 *
 * @param command The command to join
 */
void
join_command(struct Command* command, struct StringBuilder* sb);

void
join_redirect(struct Redirect* redirect, struct StringBuilder* sb);

Job*
job_new(pid_t pid, int job_number, const char* NONNULL cmdline) GCC_NONNULL(3);

void
job_delete(Job* NONNULL job) GCC_NONNULL(1);

void
job_print_imm(Job* NONNULL job) GCC_NONNULL(1);

void
job_print(Job* NONNULL job) GCC_NONNULL(1);

void
print_jobs();


Job* NULLABLE
get_job(pid_t pid);

int
register_job(pid_t job, const char* NONNULL cmdline);

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
