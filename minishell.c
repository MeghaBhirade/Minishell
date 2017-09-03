#include "common.h"
#include <signal.h>

/*Global declaration of variables*/
int status;
pid_t pid;
int ch_flag, z_flag, ctr_flag;
char buff[100], string[100];;
jobs *head = NULL;

/*function prototypes*/
void print_jobs(void);
int new_prompt(char *buff, char *new_shell);
int return_prompt(char *buff);
void check_priority(void); 
int system_call_functions(char *buff);
int check_echo(char **argv, char **env);
void insert_job(char *str);
void del_job(pid_t pid);
void child_sig_handler(int signum, siginfo_t *siginfo, void *context);

/*parse function*/
int parse_function(char ***argv, char *buff, int *idx_arr, int *argc)
{
	int index = 0, j_index = 0, k_index = 0, count = 0;
	char *str = buff;

	/*find the length of the bufffer*/
	int length =  strlen(buff);

	idx_arr[k_index++] = 0;

	/*the loop should run up to less than
	  or equal to lenght*/
	while (index <= length)
	{
		if (j_index == 0)
			/*dynamic memory allocation for arguments*/
			*argv = malloc(1* sizeof(char *));
		else
			/*if more number of arguments passed from command line 
			  then reallocate the memory*/
			*argv = realloc(*argv, (j_index + 1)*sizeof(char *));

		/*check for the space and null*/
		if (buff[index] == ' ' || buff[index] == '\0')
		{
			buff[index] = '\0';

			/*if the pipe is found replace that with NULL*/
			if (strcmp(str, "|") == 0)
			{
				idx_arr[k_index++] = j_index + 1;
				count++;
				*(*argv + j_index) = NULL;
			}
			else
			{
				/*else dynamically allocate memory*/
				*(*argv + j_index) = malloc(strlen(str) + 1);
				strcpy(*(*argv + j_index), str);
			}

			j_index++;
			str = buff + index + 1;
		}
		index++;
	}
	*argc = j_index;

	/*reallocate the memory*/
	*argv = realloc(*argv, (j_index + 1)*sizeof(char *));
	*(*argv + j_index) = NULL; 

	return count;
}

/*msh functions*/
int system_call_functions(char *buff)
{
	char *str;

	/*chcek the buffer content is pwd*/
	if (strcmp(buff, "pwd") == 0)
	{
		/*if it is pwd then
		  print current working directory*/
		getcwd(buff, 100);
		printf("%s\n", buff);
		buff[3] = '\0';

	}
	/*chcek that buffer content is mkdir*/
	else if (strcmp(buff, "mkdir") == 0)
	{
		/*it is true create new directory*/
		mkdir(buff + 6, 0777);
		buff[3] = '\0';
	}

	/*check buffer content is cd*/
	else if (strcmp(buff, "cd") == 0)
	{
		if ((buff[3]) == '\0' || strlen(buff + 3) == 14 )
		{
			/*print home directory*/
			str =  "/home/emertxe/";
			printf("%s\n", str);
			chdir(str);
		}
		else 
		{
			/*enter the corresponding directory*/
			printf("%s\n", buff + 3);
			chdir(buff + 3);
			buff[3] = '\0';
		}
	}

	/*check the buffer content is rmdir*/
	else if (strcmp(buff, "rmdir") == 0)
	{
		/*rmove the directory*/
		rmdir(buff + 6);
		buff[3] = '\0';
	}

	else 
		return 0;

	return 1;
}

/*function to return new prompt*/
int new_prompt(char *buff, char *new_shell)
{
	/*check buffer content ps1*/
	if (strncmp(buff, "PS1=", 4) == 0)
	{
		/*check for space and tab*/
		if (buff[4] != ' ' && buff[4] != '\t' )

			/*copy the newshell int to buffer*/
			strcpy(new_shell, buff + 4);
		else
			printf("PS1: command not found\n");
		return 1;
	}

	return 0;
}

/*function to return the prompt*/
int return_prompt(char *buff)
{

	/*check for control flag*/
	if (ctr_flag || strlen(buff) == 0)
	{
		/*if flag is set it returns to prompt*/
		if (ctr_flag)
		{
			printf("\n");
			ctr_flag = 0;
		}
		return 1;
	}
	else 
		/*else normal termination*/
		return 0;
}

/*function for check for echo*/
int check_echo(char **argv, char **env)
{
	int index = 0;

	/*check first argvector is echo*/
	if (strcmp(argv[0], "echo") == 0)
	{
		/*if echo followed by NULL print new line*/
		if (argv[1] == NULL)
			printf("\n");

		/*if echo followed by $? print exit status of process*/
		else if (strcmp(argv[1], "$?") == 0)
			printf("%d\n", WEXITSTATUS(status));

		/*if echo followed by $$ print pid of the process*/
		else if (strcmp(argv[1], "$$") == 0)
			printf("%d\n", getpid());

		/*if echo followed SHELL print path of directory*/
		else if (strcmp(argv[1], "$SHELL") == 0)
		{
			printf("%s\n", string);
		}
		else 

			return 0;

		return 1;
	}
	return 0;
}

/*function to check priority of process*/
void check_priority(void) 
{

	int flag1 = 0;
	jobs *ptr = head;

	if (ptr->next != NULL)
	{
		ptr->priority = '+';
		return;
	}
	/*move the pointer and set all priority
	  to space*/
	while (ptr->next !=  NULL)
	{
		ptr->priority= ' ';
		ptr = ptr->next;
	}
	ptr->priority = ' ';

	/*decalre one pointer and assign the last status of list*/
	jobs *last = ptr;

	/*search for the first stop from last of the list*/
	while (ptr->prev != NULL && strcmp(ptr->state, "Stopped") != 0)
	{
		/*move the pointer to prev*/
		ptr = ptr->prev;
	}

	/*if it is not the end of the list
	  assign the priority to the process*/
	if (ptr != NULL)
	{
		ptr->priority = '+';

		/*move the pointer to prev*/
		ptr = ptr->prev;

		/*check for the second stop fro the last*/
		while (ptr->prev != NULL && strcmp(ptr->state, "Stopped") != 0)
		{
			/*move the pointer to prev*/
			ptr = ptr->prev;

		}
		/*if not the end of the list
		  assign the priory*/
		if (ptr != NULL)
			ptr->priority = '-';

		else
		{
			/*else check for the first running state*/
			while (strcmp(last->state, "Running") != 0)
			{
				/*move last to prev*/
				last = last->prev;
				/*if it found break the loop*/
				break;
			}

			/*if it ids not end of list asssign the priority*/
			if (last != NULL)
				last->priority = '-';
		}
	}
	else
	{
		/**check for first run from the last*/
		while (last->prev != NULL && strcmp(last->state, "Running") != 0)
		{
			/*if it found break tthe loop*/
			last = last->prev;
			break;
		}	
		/*if last is not null 
		  assign the priority*/
		if (last != NULL)
		{
			last->priority = '+';

			/*move the last pointer to prev*/
			last = last->prev;

			/*chcek for the last second run*/
			while (last->prev != NULL && strcmp(last->state, "Running") != 0)
			{
				/*if it is found break the loop*/
				last = last->prev;
				break;
			}	
			/*if last pointer is not null 
			  assign the priory*/
			if (last != NULL)
				last->priority = '-';
		}
	}
}

/*function for inserting job*/
void insert_job(char *str)
{
	/*check list is empty or not*/
	if (head == NULL)
	{
		/*if it is true do dynamic memory allocation
		  and copy the all process information*/
		head = malloc(sizeof(jobs));

		head->pid =  pid;

		strcpy(head->state, str);

		strcpy(head->process_name, buff);

		if(strcmp(head->state, "Running") == 0)
			strcat(head->process_name, " &");

		head->process_num = 1;

		head->priority = '+';

		head->next = NULL;

		head->prev = NULL;
	}
	else
	{
		/*else declare one pointer assign to head*/
		jobs *ptr;
		ptr = head;

		/*move pointer to next*/
		while (ptr->next != NULL)
			ptr = ptr->next;

		/*dynamic allocation of memory*/
		ptr->next = malloc(sizeof(jobs));

		ptr->next->prev = ptr;

		ptr->next->next = NULL;

		/*copy all process information*/
		ptr->next->process_num = ptr->process_num + 1;

		strcpy(ptr->next->state, str);

		strcpy(ptr->next->process_name, buff);
		
		if(strcmp(ptr->next->state, "Running") == 0)
			strcat(ptr->next->process_name, " &");

		ptr->next->priority = ' ';

		ptr->next->pid = pid;

		/*function call to check priority*/
		check_priority();
	}
}

/*function to delete jobs*/
void del_job(pid_t pid)
{
	jobs *ptr;

	ptr = head;

	/*check pointer not equal to null and pid is
	  not equal to received pid*/
	while (ptr != NULL && ptr->pid != pid)
		ptr = ptr->next;

	/*if pointer points to null print error message*/
	if (ptr == NULL)
		printf("Process not present\n");

	else
	{
		/*if ptr notnull assign
		  pointer prev node address to pointer next node*/
		if (ptr != head)
		{
			ptr->prev->next = ptr->next;

			/*check pointer next not equal to null*/
			if (ptr->next != NULL)
				ptr->next->prev = ptr->prev;
		}

		else
			/*else move head to head next*/
			head = head->next;
		/*free that pointer */
		free(ptr);
	}
	/*function call to check priority*/
	if (head != NULL)
		check_priority();
}

/*function to print jobs*/
void print_jobs(void)
{
	/*check list is empty*/
	if (head == NULL)
		return;

	else
	{
		jobs *ptr;
		ptr = head;

		/*run the loop up to end of the list*/
		while (ptr != NULL)
		{
			/*print the process number, priority, state and name*/
			printf("[%d]%c  %s %s\n", ptr->process_num, ptr->priority, ptr->state, ptr->process_name);
			ptr = ptr->next;
		}
	}
}

/*signal handler function for child process*/
void child_sig_handler(int signum, siginfo_t *siginfo, void *context)
{
	char exit[10];
	jobs *jptr = head, *ptr;

	/*if child is exited print status*/
	if (siginfo->si_code == CLD_EXITED)
	{	
		sprintf(exit, "%d", siginfo->si_status);

		/*run loop up to NULL and pid of process found*/
		while (jptr != NULL && jptr->pid != pid)
			jptr = jptr->next;

		if (jptr != NULL)
		{
			/*copy that exit into jptr points to address of the state*/
			strcpy(jptr->state, "Exit");
			strcat(jptr->state, exit);
		}
		/*set tthe flag*/
		ch_flag = 1;
	}

	/*if child is killed call function to delete job*/
	else if (siginfo->si_code == CLD_KILLED)
	{
		/*run the loop upto the end*/
		while (jptr != NULL && jptr->pid != pid)
			jptr = jptr->next;

		/*if pointer not points to null 
		  call delete job function*/
		if (jptr != NULL)
		{
			del_job(jptr->pid);
		}
		/*set the flag*/
		ch_flag = 1;
	}

	/*check that child is stopped*/
	else if (siginfo->si_code == CLD_STOPPED)
	{
		/*traverse tthe pointer upto last*/
		while (jptr != NULL && jptr->pid != pid)
			jptr = jptr->next;

		/*run the loop up to null*/
		if (jptr != NULL)
		{
			/*copy the stopped status*/
			strcpy(jptr->state, "Stopped");

			/*if one process is stopped then print the correspointing information*/
			printf("[%d]%c  %s %s\n", jptr->process_num, jptr->priority, jptr->state, jptr->process_name);
		}
		else
		{
			/*function call to insrt the job*/
			insert_job("Stopped");

			jobs *ptr = head;
			/*move the pointer to last*/
			while (ptr->next != NULL)
			{
				ptr = ptr->next;
			}
			/*print the correspondig process information*/
			printf("[%d]%c  %s %s\n", ptr->process_num, ptr->priority, ptr->state, ptr->process_name);

		}
		ch_flag = 1;
	}

	/*if the child continued set the flag*/
	else if (siginfo->si_code == CLD_CONTINUED)
	{
		ch_flag = 0;
		ptr = head;
		while(ptr != NULL && ptr->pid != pid)
			ptr = ptr->next;

		if (ptr != NULL)
			printf("%s\n",ptr->process_name);
	}

}

/*signal handler function for control c*/
void ctrl_c_handler(int signum)
{
	/*if some process is running
	  pass the sigkill signal to kill the process*/
	if (pid != 0)
	{
		kill(pid, SIGKILL);
		/*set the flag*/
		ctr_flag = 0;	
	}
	else 
	{
		/*reset the flag*/
		ctr_flag = 1;
	}
}

/*signal handler functions for control z*/
void ctrl_z_handler(int signum)
{
	/*pass the sigstop signal to stop the process*/
	kill(pid, SIGSTOP);

}
/*function to perform fg and bg operations*/
int fg_bg_functions(void)
{
	jobs *ptr = head;

	/*check the buffer content is fg*/
	if (strcmp(buff, "fg") == 0)
	{
		if (ptr == NULL)
			printf("No process in back ground\n");

		/*traverse tthe pointer upto last*/
		while (ptr != NULL && ptr->priority != '+')
			ptr = ptr->next;

		/*if pointer not points to null*/
		if (ptr != NULL)
		{
			ch_flag = 0;
			pid = ptr->pid;
			/*copy the state and pass the sigcont signal
			  to continue the process*/
			strcpy(ptr->state, "Running");
			printf("%s\n", ptr->process_name);
			kill(ptr->pid, SIGCONT);
		}
		/*reset the falg*/
		while (!ch_flag);
		ch_flag = 0;
	}
	/*check the buffer content is bg*/
	else if (strcmp(buff, "bg") == 0)
	{
		if (ptr == NULL)
			printf("No process in back ground\n");
		else
		{
			/*traverse the pointer upto last*/
			while (ptr != NULL && ptr->priority != '+')
				ptr = ptr->next;

			/*if the pointer not points to null*/
			if (ptr != NULL)
			{
				/*copy the state and pass the sigcont signal
				  to continue the process*/
				strcpy(ptr->state, "Running");
			//	printf("[%d]%c\t%s\n", ptr->process_num, ptr->priority, ptr->process_name);
				kill(ptr->pid, SIGCONT);
			}
		}

	}
	/*check the nbuffer content is jobs*/
	else if (strcmp(buff, "jobs") == 0)
	{
		/*if it is true call function to ptint the jobs*/
		print_jobs();
	}

	else 
		return 0;

	return 1;

}

/*main function*/
int main(int argc, char **argv, char **env)
{
	int pipe_count, index_array[100], idx, jdx;
	char shell[100] = "msh";
	*argv = NULL;
	jobs *ptr;
	struct sigaction act_1, act_2, act_3;

	/*set all members of structure to 0*/
	memset(&act_1, 0, sizeof(act_1));
	memset(&act_2, 0, sizeof(act_2));
	memset(&act_3, 0, sizeof(act_3));

	act_1.sa_flags = SA_SIGINFO;

	/*assign the handler function*/
	act_1.sa_sigaction = child_sig_handler;
	act_2.sa_handler = ctrl_c_handler;

	/*pass the signals*/
	sigaction(SIGINT, &act_2, NULL);
	sigaction(SIGCHLD, &act_1, NULL);

	getcwd(string, 100);
	while (1)
	{

		act_3.sa_handler = SIG_IGN;
		sigaction(SIGTSTP, &act_3, NULL);

		/*prin the prompt*/
		printf("%s :> ", shell);
		memset(&buff, 0, sizeof(buff));

		scanf("%[^\n]", buff);
		__fpurge(stdin);

		act_3.sa_handler = ctrl_z_handler;
		sigaction(SIGTSTP, &act_3, NULL);

		/*check for exit condition*/
		if (strcmp(buff, "exit") == 0)	
			break;

		/*function calls to perform shell operations*/
		if (return_prompt(buff) || new_prompt(buff, shell)|| fg_bg_functions())
			continue;

		/*parse function to count number of the pipes*/
		pipe_count = parse_function(&argv, buff, index_array, &argc);

		int p[pipe_count][2];

		/*check for pipe count is zero*/
		if (pipe_count == 0)
		{
			/*function call for shell system calls and checking echo command*/
			if (system_call_functions(buff) || check_echo(argv, env))
				continue;

			/*creat child process*/
			switch (pid = fork())
			{
				case -1:
					/*Error handling*/
					perror("fork");
					exit(EXIT_FAILURE);

				case 0:
					/*check for & background running*/
					if (strcmp(argv[argc-1], "&") == 0)
					{
						argv[argc - 1] = NULL;
						act_2.sa_handler = SIG_IGN;
						act_3.sa_handler = SIG_IGN;
						
						/*pass the signals*/
						sigaction(SIGINT, &act_2, NULL);
						sigaction(SIGTSTP, &act_3, NULL);
						
						/*run the command*/
						execvp(argv[index_array[0]], argv + index_array[0]);

					}
					else

						/*run the command*/
						execvp(argv[index_array[0]], argv + index_array[0]);
					break;

				default:

					/*check for & background running*/
					if (strcmp(argv[argc-1], "&") == 0)
					{

						insert_job("Running");
						ptr = head;

						/*set tthe flag*/
						while (ptr->next != NULL)
							ptr = ptr->next;

						if (ptr != NULL)
							printf("[%d] %d\n", ptr->process_num, ptr->pid);

						pid = 0;
					}
					else
					{
						while(!ch_flag);
						ch_flag = 0;
					}
			}
		}
		else
		{
			dup2(0, 77);
			/*else run the cases have pipes*/
			for (idx = 0; idx < pipe_count + 1; idx++)
			{
				/*error handling*/
				if (idx != pipe_count && pipe(p[idx]) == -1)
				{
					perror("pipe");
					exit(-1);
				}
				/*crate the processs*/
				switch (pid = fork())
				{
					case -1:
						/*Error handling*/
						perror("fork");
						exit(EXIT_FAILURE);

					case 0:
						/*check for pipe count*/
						if (idx != pipe_count)	
						{
							/*close the read end
							  write and redirest using dup2*/
							close(p[idx][0]);
							dup2(p[idx][1], 1);
						}
						/*run the process spacified in path*/
						execvp(argv[index_array[idx]], argv + index_array[idx]);
						break;

					default:

						/*set the flag*/
						while (!ch_flag);
						ch_flag = 0;

						/*check for pipe count*/
						if (idx != pipe_count)
						{
							/*close the read end
							  write and redirest using dup2*/
							close(p[idx][1]);
							dup2(p[idx][0], 0);
						}

						else
							dup2(77, 0);
				}
			}
		}
	}
}
