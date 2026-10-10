#include "../include/pssim_builtins.h"
#include "../include/pssim_signals.h"
#include "../include/pssim_threads.h"
#include "../include/pssim_scheduler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int pssim_is_builtin(const char *cmd) {
    if (!cmd) return 0;
    if (strcmp(cmd, "cd") == 0 ||
        strcmp(cmd, "pwd") == 0 ||
        strcmp(cmd, "help") == 0 ||
        strcmp(cmd, "clear") == 0 ||
        strcmp(cmd, "exit") == 0 ||
        strcmp(cmd, "history") == 0 ||
        strcmp(cmd, "echo") == 0 ||
        strcmp(cmd, "env") == 0 ||
        strcmp(cmd, "setenv") == 0 ||
        strcmp(cmd, "unsetenv") == 0 ||
        strcmp(cmd, "jobs") == 0 ||
        strcmp(cmd, "telemetry") == 0 ||
        strcmp(cmd, "monitor") == 0) {
        return 1;
    }
    return pssim_is_scheduler_builtin(cmd);
}

int pssim_builtin_cd(char **argv) {
    char *target_dir = argv[1];
    if (!target_dir) {
        target_dir = getenv("HOME");
        if (!target_dir) target_dir = "/";
    }
    if (chdir(target_dir) != 0) {
        perror("%sPSSIM Built-in cd error%s");
        return 1;
    }
    getcwd(g_shell_state.current_dir, sizeof(g_shell_state.current_dir));
    return 0;
}

int pssim_builtin_pwd(char **argv) {
    (void)argv;
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("PSSIM pwd error");
    }
    return 0;
}

int pssim_builtin_help(char **argv) {
    (void)argv;
    printf("\n%s=========================================================================%s\n", PSSIM_COLOR_CYAN, PSSIM_COLOR_RESET);
    printf("%s   PROCESS SCHEDULING SIMULATOR WITH SHELL INTERFACE (PSSIM) HELP    %s\n", PSSIM_COLOR_BOLD, PSSIM_COLOR_RESET);
    printf("%s=========================================================================%s\n", PSSIM_COLOR_CYAN, PSSIM_COLOR_RESET);
    printf("%sGeneral Shell Built-in Commands:%s\n", PSSIM_COLOR_GREEN, PSSIM_COLOR_RESET);
    printf("  cd [dir]                Change current working directory\n");
    printf("  pwd                     Display current working directory\n");
    printf("  clear                   Clear terminal screen\n");
    printf("  history                 View executed command history\n");
    printf("  echo [args...]          Print arguments to stdout\n");
    printf("  env                     Print current environment variables\n");
    printf("  setenv <VAR> <VAL>      Set an environment variable\n");
    printf("  unsetenv <VAR>          Unset an environment variable\n");
    printf("  jobs                    List active background jobs\n");
    printf("  telemetry / monitor     Display concurrency and system monitoring stats\n");
    printf("  exit                    Terminate shell and scheduler session\n\n");

    printf("%sProcess Scheduling Simulator Commands:%s\n", PSSIM_COLOR_GREEN, PSSIM_COLOR_RESET);
    printf("  sched-reset             Clear process queue\n");
    printf("  sched-add <P_NAME> <ARR_TIME> <BURST_TIME> [PRIORITY]\n");
    printf("                          Add process PCB to simulator queue\n");
    printf("  sched-list              Display current process queue\n");
    printf("  sched-gen <count>       Generate random realistic process workload\n");
    printf("  sched-run <algo> [q]    Run scheduling algorithm: FCFS, SJF, SRTF, PRIORITY,\n");
    printf("                          PRIORITY_P, RR, MLFQ. [q] is time quantum for RR.\n");
    printf("  sched-gantt             Print ASCII Gantt Chart for the last simulation run\n");
    printf("  sched-metrics           Print performance metrics (Avg Wait, Turnaround, Utilization)\n");
    printf("  sched-compare [q]       Run benchmark comparison across all 7 algorithms\n");
    printf("  sched-load <file>       Load process workload from text file\n");
    printf("  sched-save <file>       Save workload to text file\n");
    printf("%s=========================================================================%s\n\n", PSSIM_COLOR_CYAN, PSSIM_COLOR_RESET);
    return 0;
}

int pssim_builtin_clear(char **argv) {
    (void)argv;
    printf("\033[H\033[2J");
    fflush(stdout);
    return 0;
}

int pssim_builtin_exit(char **argv) {
    (void)argv;
    g_shell_state.running = 0;
    return 0;
}

int pssim_builtin_history(char **argv) {
    (void)argv;
    pssim_print_history();
    return 0;
}

int pssim_builtin_echo(char **argv) {
    for (int i = 1; argv[i] != NULL; i++) {
        if (argv[i][0] == '$') {
            char *val = getenv(argv[i] + 1);
            printf("%s%s", val ? val : "", argv[i + 1] ? " " : "");
        } else {
            printf("%s%s", argv[i], argv[i + 1] ? " " : "");
        }
    }
    printf("\n");
    return 0;
}

int pssim_builtin_env(char **argv) {
    (void)argv;
    extern char **environ;
    for (char **env = environ; *env != NULL; env++) {
        printf("%s\n", *env);
    }
    return 0;
}

int pssim_builtin_setenv(char **argv) {
    if (!argv[1] || !argv[2]) {
        fprintf(stderr, "%sUsage: setenv VAR VALUE%s\n", PSSIM_COLOR_RED, PSSIM_COLOR_RESET);
        return 1;
    }
    if (setenv(argv[1], argv[2], 1) != 0) {
        perror("setenv failed");
        return 1;
    }
    return 0;
}

int pssim_builtin_unsetenv(char **argv) {
    if (!argv[1]) {
        fprintf(stderr, "%sUsage: unsetenv VAR%s\n", PSSIM_COLOR_RED, PSSIM_COLOR_RESET);
        return 1;
    }
    if (unsetenv(argv[1]) != 0) {
        perror("unsetenv failed");
        return 1;
    }
    return 0;
}

int pssim_execute_builtin(pssim_cmd_t *cmd) {
    if (!cmd || !cmd->argv || !cmd->argv[0]) return 0;

    char *name = cmd->argv[0];
    if (strcmp(name, "cd") == 0) return pssim_builtin_cd(cmd->argv);
    if (strcmp(name, "pwd") == 0) return pssim_builtin_pwd(cmd->argv);
    if (strcmp(name, "help") == 0) return pssim_builtin_help(cmd->argv);
    if (strcmp(name, "clear") == 0) return pssim_builtin_clear(cmd->argv);
    if (strcmp(name, "exit") == 0) return pssim_builtin_exit(cmd->argv);
    if (strcmp(name, "history") == 0) return pssim_builtin_history(cmd->argv);
    if (strcmp(name, "echo") == 0) return pssim_builtin_echo(cmd->argv);
    if (strcmp(name, "env") == 0) return pssim_builtin_env(cmd->argv);
    if (strcmp(name, "setenv") == 0) return pssim_builtin_setenv(cmd->argv);
    if (strcmp(name, "unsetenv") == 0) return pssim_builtin_unsetenv(cmd->argv);
    if (strcmp(name, "jobs") == 0) { pssim_print_jobs(); return 0; }
    if (strcmp(name, "telemetry") == 0 || strcmp(name, "monitor") == 0) { pssim_print_telemetry(); return 0; }

    if (pssim_is_scheduler_builtin(name)) {
        return pssim_execute_scheduler_builtin(cmd);
    }
    return -1;
}
