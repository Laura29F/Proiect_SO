#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

typedef struct {
  float latitude;
  float longitude;
} Coordinates;

typedef struct {
  int id;
  char inspector[25];
  Coordinates coordonate;
  char issue_category[30];
  int severity;
  time_t time_stamp;
  char description_text[500];
} Report;

typedef struct {
  char inspector[25];
  int score;
} InspectorScore;


int find_inspector(InspectorScore scores[], int count, const char *name)
{
  int i;

  for (i = 0; i < count; i++)
    {
      if (strcmp(scores[i].inspector, name) == 0)
	{
	  return i;
	}
    }
  return -1;
}

int main(int argc, char **argv)
{
  char path[256];
  int fd;
  Report r;
  ssize_t bytes_read;
  InspectorScore scores[100];
  int score_count = 0;
  int pos;
  int i;
  const char *district;

  if (argc != 2)
    {
      fprintf(stderr, "Eroare: lipseste numele districtului.\n");
      return 1;
    }

  district = argv[1];

  snprintf(path, sizeof(path), "%s/reports.dat", district);

  fd = open(path, O_RDONLY);
  if (fd == -1)
    {
      perror("Eroare la deschiderea fisierului reports.dat");
      return 1;
    }

  while ((bytes_read = read(fd, &r, sizeof(Report))) == sizeof(Report))
    {
      pos = find_inspector(scores, score_count, r.inspector);

      if (pos == -1)
	{
	  if (score_count >= 100)
	    {
	      fprintf(stderr, "Prea multi inspectori in districtul %s.\n", district);
	      close(fd);
	      return 1;
	    }

	  strncpy(scores[score_count].inspector, r.inspector, sizeof(scores[score_count].inspector) - 1);
	  scores[score_count].inspector[sizeof(scores[score_count].inspector) - 1] = '\0';

	  scores[score_count].score = r.severity;
	  score_count++;
	}
      else
	{
	  scores[pos].score = scores[pos].score + r.severity;
	}
    }

  if (bytes_read == -1)
    {
      perror("Eroare la citirea din reports.dat");
      close(fd);
      return 1;
    }
  close(fd);

  printf("District: %s\n", district);

  if (score_count == 0)
    {
      printf("Nu exista rapoarte in acest district.\n");
      return 0;
    }

  for (i = 0; i < score_count; i++)
    {
      printf("%s: %d\n", scores[i].inspector, scores[i].score);
    }
  
  return 0;
}
