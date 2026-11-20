/*
 * Trabalho 01: Sistemas Operacionais II — MySh (Custom POSIX Shell)
 * Autores: Joao Henrique Botelho, Joao Victor Figueredo, Paulo Santos
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <ctype.h>

/* Fallbacks POSIX para compatibilidade entre Linux (glibc) e macOS (BSD) */
#ifndef LOGIN_NAME_MAX
#ifdef _POSIX_LOGIN_NAME_MAX
#define LOGIN_NAME_MAX _POSIX_LOGIN_NAME_MAX
#else
#define LOGIN_NAME_MAX 256
#endif
#endif

#ifndef HOST_NAME_MAX
#ifdef _POSIX_HOST_NAME_MAX
#define HOST_NAME_MAX _POSIX_HOST_NAME_MAX
#else
#define HOST_NAME_MAX 256
#endif
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define MAX_ARGS 64
#define MAX_CMD_LENGTH 256

/* Obtém o diretório de trabalho atual */
char *get_current_dir(char *buffer, size_t size)
{
    char *cwd = getcwd(buffer, size);
    if (cwd == NULL)
    {
        perror("getcwd");
        exit(EXIT_FAILURE);
    }
    return cwd;
}

/* Retorna o caminho do diretório HOME do usuário atual */
char *get_home_dir(void)
{
    char *home = getenv("HOME");
    if (home == NULL)
    {
        fprintf(stderr, "HOME environment variable not set\n");
        exit(EXIT_FAILURE);
    }
    return home;
}

/* Abrevia o caminho com '~' e monta o prompt personalizado [MySh] user@host:dir$ */
char *build_prompt(const char *home)
{
    char username[LOGIN_NAME_MAX];
    if (getlogin_r(username, sizeof(username)) != 0)
    {
        const char *env_user = getenv("USER");
        strncpy(username, env_user ? env_user : "user", sizeof(username) - 1);
        username[sizeof(username) - 1] = '\0';
    }

    char hostname[HOST_NAME_MAX];
    if (gethostname(hostname, sizeof(hostname)) != 0)
    {
        strncpy(hostname, "localhost", sizeof(hostname) - 1);
        hostname[sizeof(hostname) - 1] = '\0';
    }

    char cwd[PATH_MAX];
    get_current_dir(cwd, sizeof(cwd));

    size_t home_len = strlen(home);
    if (strncmp(cwd, home, home_len) == 0 && (cwd[home_len] == '/' || cwd[home_len] == '\0'))
    {
        cwd[0] = '~';
        memmove(&cwd[1], &cwd[home_len], strlen(cwd) - home_len + 1);
    }

    size_t prompt_size = strlen(cwd) + LOGIN_NAME_MAX + HOST_NAME_MAX + 16;
    char *prompt = malloc(prompt_size);
    if (prompt == NULL)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    snprintf(prompt, prompt_size, "[MySh] %s@%s:%s$", username, hostname, cwd);
    return prompt;
}

/* Executa um comando externo via fork/execvp com suporte a encadeamento de pipes */
int run_command(char **args, int input_fd, int has_next_pipe, int *next_input_fd)
{
    if (args[0] == NULL)
    {
        return 0;
    }

    int fd[2];
    if (has_next_pipe && pipe(fd) == -1)
    {
        perror("pipe");
        return -1;
    }

    pid_t pid = fork();
    if (pid == -1)
    {
        perror("fork");
        if (has_next_pipe)
        {
            close(fd[0]);
            close(fd[1]);
        }
        return -1;
    }

    if (pid == 0)
    {
        /* Restaura comportamento padrão de sinais no processo filho */
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);

        if (input_fd != STDIN_FILENO)
        {
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        }

        if (has_next_pipe)
        {
            close(fd[0]);
            dup2(fd[1], STDOUT_FILENO);
            close(fd[1]);
        }

        if (execvp(args[0], args) < 0)
        {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (input_fd != STDIN_FILENO)
        {
            close(input_fd);
        }

        if (has_next_pipe)
        {
            close(fd[1]);
            *next_input_fd = fd[0];
        }

        int status;
        if (waitpid(pid, &status, 0) == -1)
        {
            perror("waitpid");
            return -1;
        }

        if (WIFEXITED(status))
        {
            return WEXITSTATUS(status);
        }
        else if (WIFSIGNALED(status))
        {
            return -1;
        }
    }
    return 0;
}

/* Tokeniza a entrada por espaços/tabs e orquestra pipelines separados por '|' */
int parse_args(char *input, char **args)
{
    const char *delimiters = " \t\n";
    char *token;
    int arg_index = 0;
    int next_input_fd = STDIN_FILENO;

    while ((token = strtok(input, delimiters)) != NULL)
    {
        input = NULL;

        if (strcmp(token, "|") == 0)
        {
            args[arg_index] = NULL;
            if (run_command(args, next_input_fd, 1, &next_input_fd) == -1)
            {
                return -1;
            }
            arg_index = 0;
            continue;
        }

        if (arg_index < MAX_ARGS - 1)
        {
            args[arg_index++] = token;
        }
    }

    args[arg_index] = NULL;
    if (arg_index > 0)
    {
        if (run_command(args, next_input_fd, 0, NULL) == -1)
        {
            return -1;
        }
    }

    return 0;
}

/* Handler para ignorar SIGINT (Ctrl+C) e SIGTSTP (Ctrl+Z) no processo pai do shell */
void ignore_signals(int signum)
{
    (void)signum;
}

int main(void)
{
    char input[MAX_CMD_LENGTH];
    char *args[MAX_ARGS];

    signal(SIGINT, ignore_signals);
    signal(SIGTSTP, ignore_signals);

    while (1)
    {
        char *prompt = build_prompt(get_home_dir());
        printf("%s ", prompt);
        fflush(stdout);
        free(prompt);

        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            if (feof(stdin))
            {
                printf("\n");
                exit(EXIT_SUCCESS);
            }
            perror("fgets");
            exit(EXIT_FAILURE);
        }

        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n')
        {
            input[len - 1] = '\0';
        }

        if (input[0] == '\0')
        {
            continue;
        }

        if (strcmp(input, "exit") == 0)
        {
            exit(EXIT_SUCCESS);
        }
        else if (strcmp(input, "cd") == 0 || strcmp(input, "cd ~") == 0)
        {
            if (chdir(get_home_dir()) == -1)
            {
                perror("chdir");
            }
            continue;
        }
        else if (strncmp(input, "cd ", 3) == 0)
        {
            char *path = input + 3;
            if (chdir(path) == -1)
            {
                perror("chdir");
            }
            continue;
        }

        parse_args(input, args);
    }

    return 0;
}