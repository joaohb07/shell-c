/*
Trabalho 01: Sistemas Operacionais II

Joao Henrique Botelho, Joao Victor Figueredo, Paulo Santos
*/

/* Inclui as bibliotecas necessárias para o programa */
#include <stdio.h> // Biblioteca padrão para entrada/saída de dados
#include <stdlib.h> // Biblioteca padrão para funções utilitárias
#include <unistd.h> // Biblioteca para funções do sistema POSIX
#include <string.h> // Biblioteca para manipulação de strings
#include <sys/wait.h> // Biblioteca para funções de espera do processo filho
#include <fcntl.h> // Biblioteca para manipulação de arquivos
#include <errno.h> // Biblioteca para manipulação de erros
#include <limits.h> // Biblioteca para definição de valores limite
#include <signal.h> // Biblioteca para manipulação de sinais
#include <ctype.h> // Biblioteca para funções de caracteres

#define MAX_ARGS 64 // Define o número máximo de argumentos em um comando
#define MAX_CMD_LENGTH 256 // Define a constante para o comprimento máximo de um comando


/* Obtem o diretório atual */
char *get_current_dir(char *buffer, size_t size)
{
    char *cwd = getcwd(buffer, size); // Chama a função getcwd para obter o diretório atual e armazena no buffer fornecido

    /* Verifica se houve algum erro ao obter o diretório atual */
    if (cwd == NULL) 
    {
        perror("getcwd"); // Imprime o erro ocorrido
        exit(EXIT_FAILURE); // Encerra o programa com código de erro
    }
    return cwd; // Retorna o diretório atual obtido
}

/* Retorna o caminho do diretório "home" do usuário atual */
char *get_home_dir()
{
    char *home = getenv("HOME"); // A variável "home" recebe o valor da variável de ambiente "HOME", que é definida no sistema operacional.
    /* Caso o diretório "home" do usuário não possa ser determinado */
    if (home == NULL)
    {
        exit(EXIT_FAILURE); // Encerra o programa com código de erro
    }
    return home; // Retorna o diretório "home" do usuário atual obtido
}

/* Abrevia o caminho do arquivo/diretório e gera um prompt personalizado */
char *abbreviate_path(char *path, char *home)
{    
    /* Obtendo o nome do usuário e do hospedeiro */
    char username[LOGIN_NAME_MAX];
    getlogin_r(username, sizeof(username)); // obter nome do usuário
    char hostname[HOST_NAME_MAX];
    gethostname(hostname, sizeof(hostname)); // obter nome do hospedeiro

    // Obtendo o comprimento da string home e do caminho path
    size_t home_len = strlen(home);
    size_t path_len = strlen(path);

    // Se o caminho começar com o diretório raiz e o caminho for maior que o diretório raiz, substituir a primeira ocorrência de home por "~"
    if (strncmp(path, home, home_len) == 0 && (home_len < path_len))
    {
        path[0] = '~';
        memmove(&path[1], &path[home_len], path_len - home_len + 1);
    }

    // Obter o diretório atual e criar uma cópia dele
    char cwd[MAX_CMD_LENGTH];
    get_current_dir(cwd, MAX_CMD_LENGTH);
    char *abbrev_cwd = strdup(cwd);

    // Se o diretório atual começar com o diretório raiz, substituir a primeira ocorrência de home por "~" no diretório atual
    while (strncmp(abbrev_cwd, home, home_len) == 0)
    {
        memmove(&abbrev_cwd[1], &abbrev_cwd[home_len], strlen(abbrev_cwd) - home_len + 1);
        abbrev_cwd[0] = '~';
    }

    // Alocar espaço na memória para armazenar a nova string de caminho com informações do usuário, hospedeiro e diretório atual abreviado
    char *new_path = malloc(MAX_CMD_LENGTH + LOGIN_NAME_MAX + HOST_NAME_MAX + 6);

    // Verificar se a alocação de memória foi bem-sucedida, caso contrário, exibir uma mensagem de erro e sair do programa
    if (new_path == NULL)
    {
        // fprintf(stderr, "Erro de alocacao de memoria\n");
        exit(EXIT_FAILURE); // Encerra o programa com código de erro
    }

    // Concatenar as informações do usuário, hospedeiro e diretório atual abreviado em uma nova string de caminho
    sprintf(new_path, "[MySh] %s@%s:%s$", username, hostname, abbrev_cwd);

    // Liberar a memória alocada para a cópia do diretório atual
    free(abbrev_cwd);

    // Retornar a nova string de caminho
    return new_path;
}

/*
Executa um comando com argumentos e direcionamento de entrada/saída
Recebe um array de argumentos, o file descriptor para entrada padrão, 
o file descriptor para saída padrão, e um ponteiro para o file descriptor da próxima entrada
*/
int run_command(char **args, int input_fd, int output_fd, int *next_input_fd) {
    int fd[2];
    // Cria um pipe e guarda seus file descriptors no array fd
    if (pipe(fd) == -1) {
        perror("pipe");
        return -1;
    }
    
    pid_t pid = fork();// Cria um novo processo e armazena seu PID em pid

    // Trata erros na criação do processo filho
    if (pid == -1) {
        perror("fork");
        return -1;
    }

    // Se o PID do processo filho for igual a 0, o processo é o filho
    if (pid == 0) {

        // Se o file descriptor de entrada não for o stdin, redireciona o stdin para o file descriptor de entrada e fecha o file descriptor de entrada
        if (input_fd != STDIN_FILENO) {
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        }

        // Se o file descriptor de saída não for o stdout, redireciona o stdout para o file descriptor de escrita do pipe e fecha o file descriptor de leitura do pipe
        if (output_fd != STDOUT_FILENO) {
            dup2(fd[1], STDOUT_FILENO);
            close(fd[1]);
        }

        // Fecha o file descriptor de leitura do pipe
        close(fd[0]);

        // Executa o comando com os argumentos e trata erros
        if (execvp(args[0], args) < 0) {
            perror("execvp");
            exit(EXIT_FAILURE);
        } else {
            exit(EXIT_SUCCESS);
        }


    } else {
        // Fecha o file descriptor de escrita do pipe
        close(fd[1]);
        // Se houver um ponteiro para o próximo file descriptor, armazena o file descriptor de leitura do pipe, caso contrário fecha o file descriptor de leitura do pipe
        if (next_input_fd) {
            *next_input_fd = fd[0];
        } else {
            close(fd[0]);
        }

        // Espera o processo filho terminar e armazena o status de saída em status
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid");
            return -1;
        }
        // Verifica se o processo filho terminou normalmente e retorna o status de saída
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        // Se o processo filho foi terminado por um sinal, imprime uma mensagem de erro e retorna -1
        } else if (WIFSIGNALED(status)) {
            // fprintf(stderr, "Comando terminado pelo sinal %d\n", WTERMSIG(status));
            return -1;
        }
    }
    return 0;
}

/* Recebe uma string de entrada e a divide em um vetor de strings, separadas por espaços em branco, 
tabulações e quebras de linha. Também pode redirecionar a entrada e saída padrão do programa em função de símbolos especiais, 
como pipes "|".
*/
int parse_args(char *input, char **args, int *input_fd, int *output_fd) {

    const char *delimiters = " \t\n"; //definindo os delimitadores
    char *token; //variável para armazenar cada token
    int arg_index = 0;

    int next_input_fd = input_fd ? *input_fd : STDIN_FILENO; //file descriptor para o input
    int pipe_fd[2]; //array para o pipe

    while ((token = strtok(input, delimiters)) != NULL) { //laço para iterar sobre cada token encontrado na string
        if (*token == '|') { //se encontrar um pipe
            if (pipe(pipe_fd) == -1) { //cria um pipe para comunicar processos
                perror("pipe");
                return -1;
            }

            args[arg_index] = NULL; //encerra a lista de argumentos para a execução do comando atual
            input = NULL;

            if (run_command(args, next_input_fd, pipe_fd[1], &next_input_fd) == -1) { //executa o comando
                return -1;
            }

            close(pipe_fd[1]); //fecha o descritor de escrita do pipe
            arg_index = 0; //reinicia o índice para o próximo comando
            continue;
        }

        args[arg_index++] = token; //adiciona o token atual na lista de argumentos para o comando
        input = NULL;
    }

    args[arg_index] = NULL;
    if (input_fd != NULL) { //verifica se há redirecionamento de entrada
        *input_fd = STDIN_FILENO;
    }
    if (output_fd != NULL) { //verifica se há redirecionamento de saída
        *output_fd = STDOUT_FILENO;
    }

    if (run_command(args, next_input_fd, output_fd ? *output_fd : STDOUT_FILENO, NULL) == -1) { //executa o último comando
        return -1;
    }

    return 0;
}


/*Lida com sinais, ignora CTRL + C e CTRL + Z*/
void ignore_signals(int signum) {}

int main()
{
    // Define as variáveis para armazenar a entrada, saída e argumentos da linha de comando.
    char input[MAX_CMD_LENGTH];
    char *args[MAX_ARGS];
    int input_fd = STDIN_FILENO;
    int output_fd = STDOUT_FILENO;

    // Ignora os sinais SIGINT (Ctrl+C) e SIGTSTP (Ctrl+Z) para não interromper o shell.
    signal(SIGINT, ignore_signals);
    signal(SIGTSTP, ignore_signals);

    while (1)
    {
        // Obtém o diretório atual e imprime na tela como parte do prompt do shell.
        char cwd[PATH_MAX];
        printf("%s ", abbreviate_path(get_current_dir(cwd, PATH_MAX), get_home_dir()));
        fflush(stdout);

        // Lê a linha de comando inserida pelo usuário.
        if (fgets(input, MAX_CMD_LENGTH, stdin) == NULL)
        {
            if (feof(stdin))
            {
                // Se chegou ao final do arquivo de entrada, sai do shell com status de sucesso.
                exit(EXIT_SUCCESS);
            }else{
                // Se ocorreu um erro na leitura da entrada, imprime o erro e sai do shell com status de falha.
                perror("fgets");
                exit(EXIT_FAILURE);
            }
        }

        // Remove o caractere de nova linha da entrada.
        if (input[strlen(input) - 1] == '\n')
        {
            input[strlen(input) - 1] = '\0';
        }

        // Verifica se o comando é de saida ou é um dos comandos internos do shell.
        if (strcmp(input, "exit") == 0)
        {
            // Sai do shell com status de sucesso.
            exit(EXIT_SUCCESS);
        }
        else if (strcmp(input, "cd") == 0) // Apenas continua quando cd nao possui argumentos.
        {
            continue;
        }
        else if (strcmp(input, "cd ~") == 0) // Vai para o diretorio home caso o argumento de cd seja "~".
        {
            // Muda o diretório atual para o diretório home do usuário.
            chdir(get_home_dir());
            continue;
        }
        else if (strncmp(input, "cd ", 3) == 0)
        {
            // Muda o diretório atual para o diretório especificado pelo usuário.
            char *path = input + 3;
            // Quando inexistente, retorna um erro ao usuario. Quando existe apenas continua.
            if (chdir(path) == -1)
            {
                perror("chdir");
            }
            continue;
        }

        // Analisa a entrada do usuário para obter os argumentos e as opções.
        parse_args(input, args, &input_fd, &output_fd);

        // Reseta as variáveis de entrada e saída para STDIN e STDOUT.
        input_fd = STDIN_FILENO;
        output_fd = STDOUT_FILENO;
    }
    return 0;
}