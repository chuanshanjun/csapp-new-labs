/* 
 * tsh - A tiny shell program with job control
 * 
 * <Put your name and login ID here>
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/*
 * 自己加的头文件
 */
#include <string.h>


/* Misc manifest constants */
#define MAXLINE    1024   /* max line size */
#define MAXARGS     128   /* max args on a command line */
#define MAXJOBS      16   /* max jobs at any point in time */
#define MAXJID    1<<16   /* max job ID */

/* Job states */
#define UNDEF 0 /* undefined */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */

/* 
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */

/* Global variables */
extern char **environ;      /* defined in libc */
char prompt[] = "tsh> ";    /* command line prompt (DO NOT CHANGE) */
int verbose = 0;            /* if true, print additional output */
int nextjid = 1;            /* next job ID to allocate */
char sbuf[MAXLINE];         /* for composing sprintf messages */

struct job_t {              /* The job struct */
    pid_t pid;              /* job PID */
    int jid;                /* job ID [1, 2, ...] */
    int state;              /* UNDEF, BG, FG, or ST */
    char cmdline[MAXLINE];  /* command line */
};
struct job_t jobs[MAXJOBS]; /* The job list */
/* End global variables */


/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, char **argv); 
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *jobs);
int maxjid(struct job_t *jobs); 
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *jobs, pid_t pid); 
pid_t fgpid(struct job_t *jobs);
struct job_t *getjobpid(struct job_t *jobs, pid_t pid);
struct job_t *getjobjid(struct job_t *jobs, int jid); 
int pid2jid(pid_t pid); 
void listjobs(struct job_t *jobs);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);

/*
 * main - The shell's main routine 
 */
int main(int argc, char **argv) 
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF) {
        switch (c) {
        case 'h':             /* print help message */
            usage();
	    break;
        case 'v':             /* emit additional diagnostic info */
            verbose = 1;
	    break;
        case 'p':             /* don't print a prompt */
            emit_prompt = 0;  /* handy for automatic testing */
	    break;
	default:
            usage();
	}
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT,  sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler);  /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler);  /* Terminated or stopped child */

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler); 

    /* Initialize the job list */
    initjobs(jobs);

    /* Execute the shell's read/eval loop */
    while (1) {

	/* Read command line */
	if (emit_prompt) {
	    printf("%s", prompt);
	    fflush(stdout);
	}
	if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
	    app_error("fgets error");
	if (feof(stdin)) { /* End of file (ctrl-d) */
	    fflush(stdout);
	    exit(0);
	}

	/* Evaluate the command line */
	eval(cmdline);
	fflush(stdout);
	fflush(stdout);
    } 

    exit(0); /* control never reaches here */
}
  
/* 
 * eval - Evaluate the command line that the user has just typed in
 * 
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.  
*/
void eval(char *cmdline) 
{
    char buf[MAXLINE];
    char *argv[MAXARGS];
    int bg;
    pid_t pid;

    strcpy(buf, cmdline);

    bg = parseline(buf, argv);

    // 如果不是内置命令则需要新建线程操作了
    if (!builtin_cmd(argv)) {

        // 把这些是因为 上面在执行builtin_cmd 万一遇到个 信号想直接把我杀死，所以把信号放在下面
        sigset_t mask;
        sigfillset(&mask);
        sigaddset(&mask, SIGCHLD);
        sigprocmask(SIG_BLOCK, &mask, NULL); /* Block SIGCHLD */
        if ((pid = fork()) == 0) { // chlid process
            // 使用execv执行
            sigprocmask(SIG_UNBLOCK, &mask, NULL); /* Unblock SIGCHLD */
            setpgid(0, 0); // set the child process to the 独立

            if ((execve(argv[0], argv, environ)) < 0) { 
                printf("%s: Command not found.\n", argv[0]);
                exit(0);
            }
        }

        if (!bg) {
            // 如果命令是/bin/echo 开头的 我们就不添加任务
            // if (memcmp("/bin/echo", buf, 9)) {
            //     sigprocmask(SIG_BLOCK, &mask, NULL); /* Parent process */
            //     // 往jobs添加job
            //     addjob(jobs, pid, FG, buf);
            //     sigprocmask(SIG_SETMASK, &mask, NULL); /*  Unblock SIGCHLD*/
            // }

            // waitfg(pid);
            addjob(jobs, pid, FG, cmdline);
        } else {
            // sigprocmask(SIG_BLOCK, &mask_all, NULL); /* Parent process */
            // // 往jobs添加job
            // addjob(jobs, pid, BG, buf);
            // sigprocmask(SIG_SETMASK, &prev_all, NULL); /*  Unblock SIGCHLD*/

            // // 获取jobid
            // printf("[%d] (%d) %s", pid2jid(pid), pid, buf);
            addjob(jobs, pid, BG, cmdline);
        }

        // 类似于 释放锁
        sigprocmask(SIG_UNBLOCK, &mask, NULL);

        // if bg/fg
        if (!bg) {
            // wait for fg
            waitfg(pid);
        } 
        else {
            // print for bg
            printf("[%d] (%d) %s", pid2jid(pid), pid, buf);
        }
    }

    return;
}

/* 
 * parseline - Parse the command line and build the argv array.
 * 
 * Characters enclosed in single quotes are treated as a single
 * argument.  Return true if the user has requested a BG job, false if
 * the user has requested a FG job.  
 */
int parseline(const char *cmdline, char **argv) 
{
    static char array[MAXLINE]; /* holds local copy of command line */
    char *buf = array;          /* ptr that traverses command line */
    char *delim;                /* points to first space delimiter */
    int argc;                   /* number of args */
    int bg;                     /* background job? */

    strcpy(buf, cmdline);
    buf[strlen(buf)-1] = ' ';  /* replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* ignore leading spaces */
	buf++;

    /* Build the argv list */
    argc = 0;
    if (*buf == '\'') {
	buf++;
	delim = strchr(buf, '\'');
    }
    else {
	delim = strchr(buf, ' ');
    }

    while (delim) {
	argv[argc++] = buf;
	*delim = '\0';
	buf = delim + 1;
	while (*buf && (*buf == ' ')) /* ignore spaces */
	       buf++;

	if (*buf == '\'') {
	    buf++;
	    delim = strchr(buf, '\'');
	}
	else {
	    delim = strchr(buf, ' ');
	}
    }
    argv[argc] = NULL;
    
    if (argc == 0)  /* ignore blank line */
	return 1;

    /* should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0) {
	argv[--argc] = NULL;
    }
    return bg;
}

/* 
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.  
 */
int builtin_cmd(char **argv) 
{
    if (!strcmp(argv[0], "quit")) {
        exit(0);
    }

    if (!strcmp(argv[0], "fg")) {
        do_bgfg(argv);
        return 1;
    }

    if (!strcmp(argv[0], "bg")) {
        do_bgfg(argv);
        return 1;
    }

    if (!strcmp(argv[0], "&")) {
        return 1;
    }

    // jobs命令,展示任务队列
    if (!strcmp(argv[0], "jobs")) {
        listjobs(jobs);
        return 1;
    }
    return 0;     /* not a builtin command */
}

/* 
 * do_bgfg - Execute the builtin bg and fg commands
 */
void do_bgfg(char **argv) 
{
    struct job_t *job;
    char *tmp;
    int jid;
    pid_t pid;

    tmp = argv[1];

    // if id does not exist
    // eg bg 
    if (tmp == NULL) {
        printf("%s command require PID or jobid arguments\n", argv[0]);
        return;
    }

    // if it is a jid
    if (tmp[0] == '%') {
        jid = atoi(&tmp[1]);
        // get job
        job = getjobjid(jobs, jid);
        if (job == NULL) {
            printf("%s: NO such job\n", tmp);
            return;
        } else {
            pid = job->pid;
        }
    }

    // if it is a pid
    else if(isdigit(tmp[0])) {
        // get pid
        pid = atoi(tmp);
        // get job
        job = getjobpid(jobs, pid);
        // 防止 bg 999 or fg aaa
        if (job == NULL) {
            printf("(%d) no such process\n", pid);
            return;
        }
    }

    else {
        printf("%s: argument must be a PID or jid\n", argv[0]);
        return;
    }

    // bg %1
    // kill for each time
    kill(-pid, SIGCONT); // 工作停的话，让工作重新启动

    if (!strcmp("fg", argv[0])) {
        // wait for fg
        job->state = FG;
        waitfg(job->pid);
    }
    else {
        // print for bg
        printf("[%d] [%d] %d", job->jid, job->pid, job->cmdline);
        job->state = BG;
    }

    return;
}

/* 
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid)
{
    // pid_t pid;

    // int status;
    // if (waitpid(pid, &status, 0) < 0) {
    //     unix_error("waitfg: waitpid error");
    // }

    struct job_t* job = getjobpid(jobs, pid);
    if (job == NULL)
        return;

    // 前台任务的id 和我现在 任务的id 一样，那就自旋
    while (fgpid(jobs) == pid) {

    }


    return;
}

/*****************
 * Signal handlers
 *****************/

/* 
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.  
 *  为什么不在信号处理里面做信号屏蔽呢？
 *  1 同类型的信号一次只能响应一个，然后阻塞一个(不会打断我正在做的信号处理程序)
 *  2 不同类型的信号可以互相中断
 * 
 */
void sigchld_handler(int sig) 
{
    int olderrno = errno;
    pid_t pid;
    int status;

    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        // enter here means that one of child has changed status
        if (WIFEXITED(status)) {
            // exit normally exit(0) exit(1)
            deletejob(jobs, pid);
        } else if (WIFSIGNALED(status)) {
            // terminated by signal
            deletejob(jobs, pid);
        } else if (WIFSTOPPED(status)) {
            // stops need change status
            struct job_t* job = getjobpid(jobs, pid);
            job->state = ST;
        }   
    }
    
    errno = olderrno;
    return;
}

/* 
 * sigint_handler - The kernel sends a SIGINT to the shell whenever the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.  
 */
void sigint_handler(int sig) 
{
    // 这些是自己做的
    // int olderrno = errno;
    // pid_t pid;
    // int jid;

    // sigset_t mask_all, prev_all;
    // sigfillset(&mask_all);

    // // 获取foreground pid
    // pid = fgpid(jobs);
    
    // if (!pid) {
    //     errno = olderrno;
    //     return;
    // }

    // // 获取foreground jid
    // jid = pid2jid(pid);

    // // 删除job时， 阻塞所有信号
    // sigprocmask(SIG_BLOCK, &mask_all, NULL); /* Parent process */
    // deletejob(jobs, pid);
    // sigprocmask(SIG_SETMASK, &prev_all, NULL); /*  Unblock SIGCHLD*/

    // // 发送信号 
    // kill(pid, sig);

    // char str[70];
    // sprintf(str, "Job [%d] (%d) terminated by signal %d\n", jid, pid, sig);

    // write(STDOUT_FILENO, str, strlen(str));

    // errno = olderrno;
    
    // 按照网上的教程做的
    pid_t pid = fgpid(jobs);
    if (pid == 0)
        return

    // -pid是因为process 可能会fork出其他的process
    kill(-pid, sig);

    return;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.  
 */
void sigtstp_handler(int sig) 
{
    // int olderrno = errno;

    // pid_t pid;
    // int jid;

    // // 1 获取fg pid
    // pid = fgpid(jobs);

    // if (!pid) {
    //     errno = olderrno;
    //     return;
    // }

    // jid = pid2jid(pid);

    // // 2 往pid 发送信号 
    // kill(pid, sig);

    // // 3 输出
    // char msg[70];
    // sprintf(msg, "Job [%d] (%d) stopped by signal %d\n", jid, pid, sig);
    // write(STDOUT_FILENO, msg, strlen(msg));

    // errno = olderrno;

    pid_t pid = fgpid(jobs);
    if (pid == 0)
        return;

    // 进程组
    kill(-pid, sig);
    return;
}

/*********************
 * End signal handlers
 *********************/

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(struct job_t *job) {
    job->pid = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list */
void initjobs(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	clearjob(&jobs[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(struct job_t *jobs) 
{
    int i, max=0;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid > max)
	    max = jobs[i].jid;
    return max;
}

/* addjob - Add a job to the job list */
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline) 
{
    int i;
    
    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == 0) {
	    jobs[i].pid = pid;
	    jobs[i].state = state;
	    jobs[i].jid = nextjid++;
	    if (nextjid > MAXJOBS)
		nextjid = 1;
	    strcpy(jobs[i].cmdline, cmdline);
  	    if(verbose){
	        printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
            }
            return 1;
	}
    }
    printf("Tried to create too many jobs\n");
    return 0;
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t *jobs, pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == pid) {
	    clearjob(&jobs[i]);
	    nextjid = maxjid(jobs)+1;
	    return 1;
	}
    }
    return 0;
}

/* fgpid - Return PID of current foreground job, 0 if no such job */
pid_t fgpid(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].state == FG)
	    return jobs[i].pid;
    return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
struct job_t *getjobpid(struct job_t *jobs, pid_t pid) {
    int i;

    if (pid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid)
	    return &jobs[i];
    return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
struct job_t *getjobjid(struct job_t *jobs, int jid) 
{
    int i;

    if (jid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid == jid)
	    return &jobs[i];
    return NULL;
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid) {
            return jobs[i].jid;
        }
    return 0;
}

/* listjobs - Print the job list */
void listjobs(struct job_t *jobs) 
{
    int i;
    
    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid != 0) {
	    printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
	    switch (jobs[i].state) {
		case BG: 
		    printf("Running ");
		    break;
		case FG: 
		    printf("Foreground ");
		    break;
		case ST: 
		    printf("Stopped ");
		    break;
	    default:
		    printf("listjobs: Internal error: job[%d].state=%d ", 
			   i, jobs[i].state);
	    }
	    printf("%s", jobs[i].cmdline);
	}
    }
}
/******************************
 * end job list helper routines
 ******************************/


/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void) 
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg)
{
    fprintf(stdout, "%s\n", msg);
    exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t *Signal(int signum, handler_t *handler) 
{
    struct sigaction action, old_action;

    action.sa_handler = handler;  
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
	unix_error("Signal error");
    return (old_action.sa_handler);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig) 
{
    printf("Terminating after receipt of SIGQUIT signal\n");
    exit(1);
}


