// Konstantinos Zioutos AM3946
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

#define CURRENTDIR_SIZE 1000
#define COMMAND_MAXSIZE 1000
#define COMMAND_MAXPIECES 1000

#define COMMAND_SIMPLE_TYPE 0
#define COMMAND_SEQUENCE_TYPE 1
#define COMMAND_PIPES_TYPE 2
#define COMMAND_INPUTREDIR_TYPE 3
#define COMMAND_OUTPUTREDIR_TYPE 4
#define COMMAND_OUTPUTREDIRAPPEND_TYPE 5

void show_prompt();
char *read_input();
int is_chdir(char *cmd);
char *trim_leading_and_trailing_spaces(char *s);
char *trim_extra_whitespaces(char *str);
char *final_trim(char *str);
int break_by_delim(char *str, const char *delim, char **str_array);
void exec_simple_command(char *cmd);
int exec_command(char *cmd);
void exec_commands_sequence(char **cmds, int cmd_index);
int exec_commands_pipes(char **cmds, int num_of_cmds);
int exec_command_inputredir(char **cmds);
int exec_command_outputredir(char **cmds);
int exec_command_outputredirappend(char **cmds);

extern int errno;
int command_type;

int main()
{
    while (1)
    {
        command_type = COMMAND_SIMPLE_TYPE;
        show_prompt();
        char *cmd = read_input();

        while (!strcmp(cmd, "wrong"))
        { /*lathos input opote to zhtaw mexri na dothei swsto*/
            printf("False Input\n");
            show_prompt();
            cmd = read_input();
        }
        if (!strcmp(cmd, "exit") && cmd[4] == '\0') /*periptwsh exit*/
        {
            break;
        }
        else if (!strncmp(cmd, "exit", 4) && cmd[4] == ' ') /*periptwsh exit mm diladi exit kai meta keno*/
        {
            break;
        }
        else if (is_chdir(cmd)) /*ti kanw me to cd*/
        {
            if (cmd[2] == '\0')
            {
                if (chdir((const char *)getenv("HOME"))) /*periptwsh cd xoris orisma pou mas paei sto home directory*/
                {
                    printf("No such file or directory\n");
                }
            }
            else
            {
                if (chdir((const char *)&cmd[3])) /*cd me orisma*/
                {
                    printf("No such file or directory\n");
                }
            }
        }
        else /*periptwseis gia apli entoli,gia diadoxikes entoles,gia pipes kai ola ta redirections*/
        {
            if (command_type == COMMAND_SIMPLE_TYPE)
            {
                exec_simple_command(cmd);
            }
            else if (command_type == COMMAND_SEQUENCE_TYPE)
            {
                char *broken_cmd[COMMAND_MAXPIECES];
                break_by_delim(cmd, ";", broken_cmd);
                exec_commands_sequence(broken_cmd, 0);
            }
            else if (command_type == COMMAND_PIPES_TYPE)
            {
                char *broken_cmd[COMMAND_MAXPIECES];
                int num_of_cmds = break_by_delim(cmd, "|", broken_cmd);
                exec_commands_pipes(broken_cmd, num_of_cmds);
            }
            else if (command_type == COMMAND_INPUTREDIR_TYPE)
            {
                char *broken_cmd[COMMAND_MAXPIECES];
                int num_of_cmds = break_by_delim(cmd, "<", broken_cmd);
                if (exec_command_inputredir(broken_cmd) != 0)
                {
                    printf("Error\n");
                }
            }
            else if (command_type == COMMAND_OUTPUTREDIR_TYPE)
            {
                char *broken_cmd[COMMAND_MAXPIECES];
                int num_of_cmds = break_by_delim(cmd, ">", broken_cmd);
                exec_command_outputredir(broken_cmd);
            }
            else if (command_type == COMMAND_OUTPUTREDIRAPPEND_TYPE)
            {
                char *broken_cmd[COMMAND_MAXPIECES];
                int num_of_cmds = break_by_delim(cmd, ">>", broken_cmd);
                exec_command_outputredirappend(broken_cmd);
            }
        }
    }
    return 0;
}

void show_prompt() /*deixnoume to prompt tou shell*/
{
    char currentdir[CURRENTDIR_SIZE];
    printf("%s@cs345sh/%s$ ", getlogin(), getcwd(currentdir, CURRENTDIR_SIZE));
}

char *read_input()
{
    // trim deksia, aristera dhl oxi kena stis akries
    // endiamesa oxi pollapla kena, dhl oxi "wc    file.txt", alla "wc file.txt"
    //; | < > >> xvriw kena deksia, aristera
    // ls | wc --> ls|wc
    //"            cd       newdir" --> "cd newdir"
    int c, i = 0, f = 0;
    char *cmd = (char *)malloc(1000 * sizeof(char));
    char *cmd_after_trim = (char *)malloc(1000 * sizeof(char));
    char *cmd_afer_trim_extra_spaces = (char *)malloc(1000 * sizeof(char));
    char *final_cmd = (char *)malloc(1000 * sizeof(char));
    while ((c = getchar()) != '\n')
    {
        cmd[i] = c;
        if (cmd[i] == ';' || cmd[i] == '|' || cmd[i] == '<' || cmd[i] == '>') /*ksexorizw tis periptwseis twn entolwn plin aplwn*/
        {
            f = 1; /*flag an uparxei tetoia periptwsi*/
            if (cmd[i] == ';')
            {
                command_type = COMMAND_SEQUENCE_TYPE;
            }
            else if (cmd[i] == '|')
            {
                command_type = COMMAND_PIPES_TYPE;
            }
            else if (cmd[i] == '<')
            {
                command_type = COMMAND_INPUTREDIR_TYPE;
            }
            else if (cmd[i] == '>' && cmd[i - 1] != '>')
            {
                command_type = COMMAND_OUTPUTREDIR_TYPE;
            }
            else if (cmd[i] == '>' && cmd[i - 1] == '>')
            {
                command_type = COMMAND_OUTPUTREDIRAPPEND_TYPE;
            }
            if ((cmd[i - 1] == cmd[i] && cmd[i] != '>') || (cmd[i - 2] == cmd[i - 1] && cmd[i - 1] == cmd[i] && cmd[i] == '>'))
            {
                f = 2; /*flag an einai synexomena*/
            }
        }

        i++;
    }
    cmd[i] = '\0';
    cmd_after_trim = trim_leading_and_trailing_spaces(cmd);
    cmd_afer_trim_extra_spaces = trim_extra_whitespaces(cmd_after_trim);

    if (f == 1) /*otan yparxei mia periptwsi apo tis parapanw*/
    {
        if ((cmd[0] == ';' || cmd[0] == '|' || cmd[0] == '<' || cmd[0] == '>') || (cmd[i - 1] == '|' || cmd[i - 1] == '<' || cmd[i - 1] == '>'))
        {
            return "wrong"; /*periptwseis gia lathos input*/
        }
        if (cmd[i - 1] == ';')
        { /*; sto telos paraliptetai*/
            cmd[i - 1] = '\0';
        }
        final_cmd = final_trim(cmd_afer_trim_extra_spaces);
        return final_cmd; /*epistrofi tis telikhs morfis meta to trimarisma kai stis periptwseis me ta eidika symbola*/
    }
    else if (f == 2)
    { /*Synexomena symbola ektos tou >> epistrofi lathos input*/
        return "wrong";
    }
    return cmd_afer_trim_extra_spaces;
}

char *trim_leading_and_trailing_spaces(char *str) /*trimarw ta kena kai ta tab sthn arxi kai sto telos*/
{
    int i = 0, j = 0, k = 0;

    while (str[i] == ' ' || str[i] == '\t')
    {
        i++;
    }

    while (str[i] != '\0')
    {
        str[j++] = str[i];
        i++;
    }

    str[j] = '\0';
    while (str[k] != '\0')
    {
        if (str[k] != ' ' || str[i] == '\t')
        {
            j = k;
        }
        k++;
    }
    str[j + 1] = '\0';
    return str;
}

char *trim_extra_whitespaces(char *str) /*otan yparxoun pollapla kena kai tabs kai prepei na ginei ena mono*/
{
    int i = 0, j = 0;
    while (str[i] != '\0')
    {
        if ((str[i] == ' ' || str[i] == '\t') && (str[i - 1] == ' ' || str[i - 1] == '\t'))
        {
            i++;
            continue;
        }
        str[j] = str[i];
        j++;
        i++;
    }
    str[j] = '\0';
    return str;
}

char *final_trim(char *str) /*trimarisma gia ta extra symbola deksia kai aristera afairw ta kena*/
{
    int i, j, len = strlen(str);

    for (i = 0; str[i] != '\0'; i++)
    {
        if (str[i] == ';' || str[i] == '|' || str[i] == '<' || str[i] == '>')
        {
            if (str[i - 1] == ' ' || str[i - 1] == '\t') // Exw treis periptwseis Gia na kalyptw kathe pithani thesi toy kenou
            {                                            // akoma kai an exei peiraxtei h thesi apo proigoumeni periptosi
                for (j = i - 1; str[j] != '\0'; j++)
                {
                    str[j] = str[j + 1];
                }
                len--;
                i--;
            }
            if (str[i] == ' ' || str[i] == '\t')
            {
                for (j = i; str[j] != '\0'; j++)
                {
                    str[j] = str[j + 1];
                }
                len--;
                i--;
            }
            if (str[i + 1] == ' ' || str[i + 1] == '\t')
            {
                for (j = i + 1; str[j] != '\0'; j++)
                {
                    str[j] = str[j + 1];
                }
                len--;
            }
            str[len] = '\0';
        }
    }
    return str;
}

int is_chdir(char *cmd)
{
    if (!strncmp(cmd, "cd ", 2) && cmd[2] == '\0')
    {
        return 1;
    }
    return !strncmp(cmd, "cd ", 3);
}

int break_by_delim(char *str, const char *delim, char **str_array) /*na xorizw ena string me vasi ena symbolo*/
{
    int counter = 0;
    char *token;
    token = strtok(str, delim);
    while (token != NULL)
    {
        str_array[counter++] = token;
        token = strtok(NULL, delim);
    }
    str_array[counter] = NULL;
    return counter;
}


void exec_simple_command(char *cmd) /*ektelesi aplis entolis*/
{
    pid_t cpid;
    if ((cpid = fork()) == 0)
    {
        exec_command(cmd);
    }
    else
    {
        cpid = wait(NULL);
    }
}

int exec_command(char *cmd) /*ektelesi kathe entolis me thn execvp*/
{
    char *broken_cmd[COMMAND_MAXPIECES];
    break_by_delim(cmd, " ", broken_cmd);
    if(execvp(broken_cmd[0], broken_cmd)==-1){
        printf("Error\n");
        return errno;
    }
    return 0;
}

void exec_commands_sequence(char **cmds, int cmd_index)
{ /*anadromi gia thn ylopoihsh diadoxikwn entolwn xorismenwn me ";" */
    pid_t cpid;

    if (cmds[cmd_index] == NULL)
    {
        return;
    }

    if ((cpid = fork()) == 0)
    {
        exec_command(cmds[cmd_index]);
    }
    else
    {
        cpid = wait(NULL);
        exec_commands_sequence(cmds, cmd_index + 1);
    }
}

int exec_commands_pipes(char **cmds, int num_of_cmds)
{ /*gia ta pipes*/
    int i = 0;
    int pipefd[num_of_cmds][2]; /*disdiastato pinaka sto prwto exei megethos oses entoles exoume mporei duo mporei kai parapanw giati ypostirizei kai multiple,sti deuteri einai duo(input output)*/
    pid_t cpid;

    while (cmds[i] != NULL)
    {
        if (pipe(pipefd[i]) == -1)
        {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        if ((cpid = fork()) == 0)
        {               /*periptwseis gia ta pipes*/
            if (i == 0) /* To teleutaio command sthn akolouthia pipe diavazei apo to stdin, alla grafei sto write end apo to prwto pipe (pipefd[0][1])*/
            {
                if (dup2(pipefd[i][1], STDOUT_FILENO) == -1) /*diaxeirisi error*/
                {
                    return errno;
                }
                close(pipefd[i][0]);
            }
            else if (i == num_of_cmds - 1) /*To teleutaio command sthn akolouthia pipe diavazei apo to read end of the last pipe (pipefd[cmd][0]) kai grafei sto stdout*/
            {
                dup2(pipefd[i - 1][0], STDIN_FILENO);
                close(pipefd[i][0]);
            }
            else /* Ola ta ypoloipa diavazoun apo to read end tou proigomenou pipe (pipefd[i-1][0]) kai grafoun sto write end of tou epomenou pipe (pipefd[i][1]) */
            {
                dup2(pipefd[i - 1][0], STDIN_FILENO);
                dup2(pipefd[i][1], STDOUT_FILENO);
                close(pipefd[i - 1][0]);
                close(pipefd[i][0]);
            }
            exec_command(cmds[i]);
        }
        else
        {
            cpid = wait(NULL);
            close(pipefd[i][1]);
        }
        i++;
    }
    return 0;
}

int exec_command_inputredir(char **cmds)
{ /* input redirection, vazw tis 2 entoles se ena pinaka*/
    pid_t cpid;
    if ((cpid = fork()) == 0)
    {
        int fd;
        if ((fd = open(cmds[1], O_RDONLY)) == -1)
        {
            return errno;
        }
        if (dup2(fd, STDIN_FILENO) == -1)
        { /*kalw thn dup2*/
            return errno;
        }
        exec_command(cmds[0]);
        close(fd);
    }
    else
    {
        cpid = wait(NULL);
    }

    return 0;
}

int exec_command_outputredir(char **cmds)
{ /*antistoixa me thn input*/
    pid_t cpid;
    if ((cpid = fork()) == 0)
    {

        int fd;
        if ((fd = open(cmds[1], O_CREAT | O_WRONLY)) == -1)
        {
            return errno;
        }
        if (dup2(fd, STDOUT_FILENO) == -1)
        {
            return errno;
        }
        
        exec_command(cmds[0]);
        close(fd);
    }
    else
    {
        cpid = wait(NULL);
    }

    return 0;
}

int exec_command_outputredirappend(char **cmds)
{ /*antistoixa me thn output apla bazw kai to O_append)*/
    pid_t cpid;
    if ((cpid = fork()) == 0)
    {
        int fd;
        if ((fd = open(cmds[1], O_CREAT | O_WRONLY | O_APPEND)) == -1)
        {
            return errno;
        }
        if (dup2(fd, STDOUT_FILENO) == -1)
        {
            return errno;
        }
        exec_command(cmds[0]);
        close(fd);
    }
    else
    {
        cpid = wait(NULL);
    }

    return 0;
}