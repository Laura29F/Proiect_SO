#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_LINE 512
#define MAX_ARGS 32
#define BUFFER_SIZE 1024


int parse_command(char *line, char *args[])
{
  int count = 0;
  char *token;

  token = strtok(line, " \t\n");

  while (token != NULL && count < MAX_ARGS)
    {
      args[count] = token;
      count++;
      token = strtok(NULL, " \t\n");
    }
  return count;
}

void run_scorer_for_district(const char *district)
{
  int pipe_fd[2];
  pid_t pid;
  char buffer[BUFFER_SIZE];
  ssize_t bytes_read;
  int status;

  if (pipe(pipe_fd) == -1)
    {
      perror("Eroare la pipe");
      return;
    }

  pid = fork();

  if (pid == -1)
    {
      perror("Eroare la fork");
      close(pipe_fd[0]);
      close(pipe_fd[1]);
      return;
    }

  if (pid == 0)
    {
      close(pipe_fd[0]);

      if (dup2(pipe_fd[1], STDOUT_FILENO) == -1)
	{
	  perror("Eroare la dup2");
	  exit(1);
	}
      close(pipe_fd[1]);

      execlp("./score_reports", "score_reports", district, (char *)NULL);

      perror("Eroare la execlp pentru score_reports");
      exit(1);
    }
  
  close(pipe_fd[1]);

  printf("\n== Scoruri pentru districtul %s ==\n", district);

  while ((bytes_read = read(pipe_fd[0], buffer, sizeof(buffer) -1)) > 0)
    {
      buffer[bytes_read] = '\0';
      printf("%s", buffer);
    }

  if (bytes_read == -1)
    {
      perror("Eroare la citirea din pipe");
    }
  close(pipe_fd[0]);

  if (waitpid(pid, &status, 0) == -1)
    {
      perror("Eroare la waitpid");
      return;
    }

  if (!(WIFEXITED(status) && WEXITSTATUS(status) == 0))
    {
      printf("Scorer-ul pentru districtul %s nu s-a terminat cu succes.\n", district);
    }
}

void handle_calculate_scores(int argc, char *args[])
{
  int i;

  if (argc < 2)
    {
      printf("Eroare: lipsa nume district.\n");
      return;
    }
  printf("\nRaport combinat de workload:\n");

  for (i = 1; i < argc; i++)
    {
      run_scorer_for_district(args[i]);
    }
}

void handle_start_monitor(void)
{

  printf("Comanda start_monitor a fost recunoscuta.\n");
}

int main(void)
{
  char line[MAX_LINE];
  char *args[MAX_ARGS];
  int argc;

  printf("City Hub pornit.\n");

  while (1)
    {
      printf("\ncity_hub> ");

      if (fgets(line, sizeof(line), stdin) == NULL)
	{
	  printf("\nCity Hub se inchide.\n");
	  break;
	}
      argc = parse_command(line, args);

      if (argc == 0)
	{
	  continue;
	}

      if (strcmp(args[0], "exit") == 0)
	{
	  printf("City Hub se inchide.\n");
	  break;
	}
      else if (strcmp(args[0], "calculate_scores") == 0)
	{
	  handle_calculate_scores(argc, args);
	}
      else if (strcmp(args[0], "start_monitor") == 0)
	{
	  handle_start_monitor();
	}
      else
	{
	  printf("Comanda necunoscuta: %s\n", args[0]);
	}
    }

  return 0;
}
