#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define	STD_IN	0
#define	STD_OUT	1
#define	STD_ERR	2

#define	TYPE_OUT	0
#define	TYPE_IN		1

#define	TYPE_END	0
#define	TYPE_BREAK	1
#define	TYPE_PIPE	2

typedef	struct		s_list
{
	char			**commands;
	int				pipe[2];
	int				type;
	int				len;
	struct s_list 	*next;
	struct s_list 	*previous;
}					t_list;

int     ft_strlen(char *str)
{
    int i = 0;

    while (str[i] != 0)
        i++;
    return (i);
}

char    *ft_strdup(char *str)
{
	int 	i = 0;
	char	*ret;

	ret = (char*)malloc(sizeof(char) * (ft_strlen(str) + 1));
	while (i < ft_strlen(str))
	{
		ret[i] = str[i];
		i++;
	}
	ret[i] = '\0';
	return (ret);
}

void	add_type(t_list **cmds, int type)
{
	t_list	*list;

	list = *cmds;
	while (list->next != NULL)
		list = list->next;
	list->type = type;
}

void	add_arg(t_list **cmds, char *arg)
{
	int		i = 0;
	char	**commands;
	t_list	*list;

	list = *cmds;
	while (list->next != NULL)
		list = list->next;
	commands = (char**)malloc(sizeof(char*) * (list->len + 2));
	while (i < list->len)
	{
		commands[i] = list->commands[i];
		i++;
	}
	commands[i++] = ft_strdup(arg);
	//printf("I'm here\n");
	commands[i] = 0;
	free(list->commands);
	list->commands = commands;
	list->len++;
}

void		list_push(t_list **cmds, char *arg, int type)
{
	t_list	*list;
	t_list	*new;

	new = (t_list*)malloc(sizeof(t_list));
	new->commands = NULL;
	new->type = TYPE_END;
	new->pipe[0] = 0;
	new->pipe[1] = 0;
	new->next = NULL;
	new->len = 0;
	if (*cmds == NULL)
	{
		*cmds = new;
		(*cmds)->previous = NULL;
	}
	else
	{
		list = *cmds;
		while (list->next != NULL)
			list = list->next;
		new->previous = list;
		list->next = new;
	}
	if (type > 0)
		add_type(cmds, type);
	else if (arg != NULL)
		add_arg(cmds, arg);
}

void		parse_args(t_list **cmds, char *arg)
{
	int	type = 0;

	if (strcmp(arg, ";") == 0)
		type = TYPE_BREAK;
	else if (strcmp(arg, "|") == 0)
		type = TYPE_PIPE;
	if (!(*cmds))
		list_push(cmds, arg, type);
	else if (type > 0)
	{
		add_type(cmds, type);
		list_push(cmds, NULL, 0);
	}
	else
		add_arg(cmds, arg);
	//printf("Inside parse args after first push\n");
}

void	exec_cmd(t_list **cmd, char **envp)
{
	int		status;
	pid_t	pid;

	//printf("Inside exec_cmd\n");
	if ((*cmd)->type == TYPE_PIPE || ((*cmd)->previous != NULL && (*cmd)->previous->type == TYPE_PIPE))
	{
		//printf("Creating pipe\n");
		pipe((*cmd)->pipe);
	}
	pid = fork();
	//printf("--------------------------------------\n");
	//printf("Arg0 = %s\n", (*cmd)->commands[0]);
	if (pid == 0)
	{
		//printf("Inside exec_cmd process\n");
		//printf("Arg0 = %s\n", (*cmd)->commands[0]);
		if ((*cmd)->type == TYPE_PIPE)
		{
			//printf("Inside exec_cmd pipe\n");
			dup2((*cmd)->pipe[TYPE_IN], STD_OUT);
		}
		if ((*cmd)->previous && (*cmd)->previous->type == TYPE_PIPE)
		{
			//printf("I'm here\n");
			dup2((*cmd)->previous->pipe[TYPE_OUT], STD_IN);
		}
		if (execve((*cmd)->commands[0], (*cmd)->commands, envp) == -1)
		{
			//printf("Inside execve process\n");
			write(STD_ERR, "error: cannot execute ", ft_strlen("error: cannot execute "));
			write(STD_ERR, (*cmd)->commands[0], ft_strlen((*cmd)->commands[0]));
			write(STD_ERR, "\n", 1);
		}
	}
	else if (pid < 0)
	{
		write(STD_ERR, "error: fatal\n", ft_strlen("error: fatal\n"));
		exit(1);
	}
	else
	{
		waitpid(pid, &status, 0);
		if ((*cmd)->previous && (*cmd)->previous->type == TYPE_PIPE)
			close((*cmd)->previous->pipe[0]);
		if ((*cmd)->type == TYPE_PIPE)
			close((*cmd)->pipe[1]);
	}
}

void	exec_cmds(t_list **cmds, char **envp)
{
	t_list	*list;

	list = *cmds;
	while (list != NULL)
	{
		//printf("Arg0 = %s\n", list->commands[0]);
		//printf("wooooo\n");
		if (list->commands != NULL && strcmp(list->commands[0], "cd") == 0)
		{
			if (list->len > 2 || list->len == 1)
				write(STD_ERR, "error: cd: bad arguments\n", ft_strlen("error: cd: bad arguments\n"));
			else if (chdir(list->commands[1]) == -1)
			{
				write(STD_ERR, "error: cd: cannot change directory to ", ft_strlen("error: cd: cannot change directory to "));
				write(STD_ERR, list->commands[1], ft_strlen(list->commands[1]));
				write(STD_ERR, "\n", 1);
			}
		}
		else if (list->commands != NULL)
			exec_cmd(&list, envp);
		list = list->next;
	}
}

int		main(int argc, char **argv, char **envp)
{
	int		i = 1;
	t_list	*cmds = NULL;

	while (i < argc)
		parse_args(&cmds, argv[i++]);
	//printf("Arg0 = %s\n", cmds->commands[0]);
	//printf("I'm here\n");
	exec_cmds(&cmds, envp);
	return (0);
}