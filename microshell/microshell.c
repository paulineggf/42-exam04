#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

# define PIPE 1

# define SIDE_OUT 0
# define SIDE_IN 1

# define STDIN 0
# define STDOUT 1
# define STDERR 2

typedef struct      s_list
{
    char            **args;
    int             type;
    int             pipe[2];
    struct s_list   *prev;
    struct s_list   *next;
}                   t_list;

void    lst_clear(t_list **begin_list)
{
    t_list  *tmp;
    int     i;

    while (*begin_list)
    {
        i = 0;
        tmp = (*begin_list)->next;
        while ((*begin_list)->args[i])
            free((*begin_list)->args[i++]);
        free((*begin_list)->args);
        free(*begin_list);
        *begin_list = tmp;
    }
    free(*begin_list);
}

int     ft_strlen(char *str)
{
    int     i;

    i = 0;
    while (str[i])
        i++;
    return (i);
}

char    *ft_strdup(char *str)
{
    char    *new;
    int     i;

    i = 0;
    new = malloc(sizeof(char) * (ft_strlen(str) + 1));
    while (str[i])
    {
        new[i] = str[i];
        i++;
    }
    new[i] = '\0';
    return (new);
}

void    add_args(t_list **cmds, char **argv, int *i)
{
    int     j;

    j = *i;
    while (argv[j] && strcmp(argv[j], "|") && strcmp(argv[j], ";"))
        j++;
    (*cmds)->args = malloc(sizeof(char**) * (j - *i + 1));
    (*cmds)->args[j - *i] = NULL;
    j = 0;
    while (argv[*i] && strcmp(argv[*i], "|") && strcmp(argv[*i], ";"))
    {
        (*cmds)->args[j] = ft_strdup(argv[*i]);
        j++;
        (*i)++;
    }
    if (argv[*i] && strcmp(argv[*i], "|") == 0)
        (*cmds)->type = PIPE;
    if (argv[*i])
        (*i)++;
    return ;
}

t_list  *parser(char **argv)
{
    t_list  *cmds;
    t_list  *begin_list;
    int     i;

    i = 1;
    cmds = malloc(sizeof(t_list));
    cmds->args = NULL;
    cmds->type = 0;
    cmds->prev = NULL;
    cmds->next = NULL;
    begin_list = cmds;
    while (argv[i])
    {
        add_args(&cmds, argv, &i);
        if (argv[i])
        {
            cmds->next = malloc(sizeof(t_list));
            cmds->next->prev = cmds;
            cmds = cmds->next;
            cmds->args = NULL;
            cmds->type = 0;
            cmds->next = NULL;
        }
    }
    return (begin_list);
}

void    exit_fatal(char *str, t_list **begin_list)
{
    write(STDERR, str, ft_strlen(str));
    lst_clear(begin_list);
    exit(EXIT_FAILURE);
}

int     cd(t_list *cmds)
{
    if (cmds->args[1] == NULL)
    {
        write(STDERR, "error: cd: bad arguments\n", ft_strlen("error: cd: bad arguments\n"));
        return (EXIT_FAILURE);
    }
    if (chdir(cmds->args[1]))
    {
        write(STDERR, "error: cd: cannot change directory to ", ft_strlen("error: cd: cannot change directory to "));
        write(STDERR, cmds->args[1], ft_strlen(cmds->args[1]));
        write(STDERR, "\n", 1);
        return (EXIT_FAILURE);
    }
    return (EXIT_SUCCESS);
}

int     exec_cmds(t_list *cmds, t_list **begin_list, char **env)
{
    int     pipe_open;
    int     pid;
    int     status;
    int     ret;

    pipe_open = 0;
    ret = EXIT_FAILURE;
    if (cmds->type == PIPE || (cmds->prev && cmds->prev->type == PIPE))
    {
        pipe_open = 1;
        if (pipe(cmds->pipe))
            exit_fatal("error: fatal\n", begin_list);
    }
    if (cmds->args[0] && strncmp(cmds->args[0], "cd", 3) == 0)
        ret = cd(cmds);
    else
    {
        pid = fork();
        if (pid < 0)
            exit_fatal("error: fatal\n", begin_list);
        else if (pid == 0)
        {
            if (cmds->type == PIPE
                && dup2(cmds->pipe[SIDE_IN], STDOUT) < 0)
                exit_fatal("error: fatal\n", begin_list);
            if (cmds->prev && cmds->prev->type == PIPE
                && dup2(cmds->prev->pipe[SIDE_OUT], STDIN) < 0)
                exit_fatal("error: fatal\n", begin_list);
            if (cmds->args[0])
            {
                if ((ret = execve(cmds->args[SIDE_OUT], cmds->args, env)) < 0)
                {
                    write(2, "error: cannot execute ", 22);
                    write(2, cmds->args[0], ft_strlen(cmds->args[0]));
                    write(2, "\n", 1);
                }
            }
            exit(ret);
        }
        else
        {
            waitpid(-1, &status, 0);
            if (pipe_open)
            {
                close(cmds->pipe[SIDE_IN]);
                if (cmds->next == NULL || cmds->type == 0)
                    close(cmds->pipe[SIDE_OUT]);
            }
            if (cmds->prev && cmds->prev->type == PIPE)
                close(cmds->prev->pipe[SIDE_OUT]);
            if (WIFEXITED(status))
                ret = WEXITSTATUS(status);
        }
    }
    return (ret);
}

int     main(int argc, char **argv, char **env)
{
    t_list  *cmds;
    t_list  *begin_list;
    int     ret;

    if (argc > 1)
    {
        cmds = parser(argv);
        begin_list = cmds;
        while (cmds)
        {
            ret = exec_cmds(cmds, &begin_list, env);
            cmds = cmds->next;
        }
        lst_clear(&begin_list);
        return (ret);
    }
    return (EXIT_SUCCESS);
}
