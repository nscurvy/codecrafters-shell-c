//
// Created by nkinder on 8/16/26.
//

#pragma once
#include "nullability.h"
#include "shellerr.h"

#include <signal.h>


struct AndOr;
struct Pipeline;
struct Command;
struct Redirect;
struct StringBuilder;

typedef enum JobStatus { JOB_RUNNING, JOB_STOPPED, JOB_DONE } JobStatus;

typedef struct Job {
    const char* cmdline;
    pid_t       pid;
    pid_t       pgid;
    int         job_number;
    JobStatus   status;
    int         exit_code;
    int         term_signal;
} Job;

ASSUME_NONNULL_BEGIN

void
init_shell_job_control();

void
job_control_child_setup(pid_t pipeline_pgid, bool foreground);
pid_t
job_control_parent_setup(pid_t child_pid, pid_t pipeline_pgid);

extern volatile sig_atomic_t child_exited_flag;
extern pid_t                 shell_pgid;
extern int                   shell_terminal;

/**
 * Join a AndOr node into a string representing a normalized view of the cmdline text
 * that would execute that AndOr.
 *
 * @param and_or The AndOr to join
 * @return A string representing the AndOr.
 */
const char*
join_andor(struct AndOr* and_or) GCC_NONNULL(1);

/**
 * Join a Pipeline node into a string representing a normalized view of the cmdline text
 * that would execute that pipeline.
 *
 * @param pipeline The pipeline to join
 */
void
join_pipeline(struct Pipeline* pipeline, struct StringBuilder* sb) GCC_NONNULL(1, 2);

/**
 * Join a Command node into a string representing a normalized view of the cmdline text
 * that would execute that command.
 *
 * @param command The command to join
 */
void
join_command(struct Command* command, struct StringBuilder* sb) GCC_NONNULL(1, 2);

void
join_redirect(struct Redirect* redirect, struct StringBuilder* sb) GCC_NONNULL(1, 2);

Job*
job_new(pid_t pid, pid_t pgid, int job_number, const char* cmdline, JobStatus status, int exit_code, int term_signal)
        GCC_NONNULL(4);
#define job_default(pid, pgid, job_number, cmdline, status)                                                            \
    job_new((pid), (pgid), (job_number), (cmdline), (status), 0, 0)
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

Job*
register_job(pid_t job, pid_t pgid, const char* NONNULL cmdline, JobStatus status) GCC_NONNULL(3);

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

struct JobList;

Job*
get_job_by_number(struct JobList* list, int n) GCC_NONNULL(1);
Job*
resolve_job_spec(struct JobList* list, const char* spec, ShellError* err) GCC_NONNULL(1, 2, 3);

ASSUME_NONNULL_END
