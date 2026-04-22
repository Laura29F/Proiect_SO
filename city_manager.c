#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef struct{
  float latitude;
  float longitude;
} Coordinates;

enum security_level{
  not,
  minor,
  moderate,
  critical
};

typedef struct{

  int id;
  char inspector[25];
  Coordinates coordonate;
  char issue_category[30];
  enum security_level level;
  time_t time_stamp;
  char description_text[500];
} Report;


//sa verific daca exista districtul cu fisierele - trebuie functie

void set_mode(mode_t mod, char permisiuni[10])
{
  int i;
  for (i = 0; i <= 8; i++)
    {
      permisiuni[i] = '-';
    }

  if (permisiuni[0] & S_IRUSR)
    permisiuni[0] = 'r';

  if (permisiuni[1] & S_IWUSR)
    permisiuni[1] = 'w';

  if (permisiuni[2] & S_IXUSR)
    permisiuni[2] = 'x';
  
  if (permisiuni[3] & S_IRUSR)
    permisiuni[3] = 'r';

  if (permisiuni[4] & S_IWUSR)
    permisiuni[4] = 'w';
   
  if (permisiuni[5] & S_IXUSR)
    permisiuni[5] = 'x';
  
  if (permisiuni[6] & S_IRUSR)
    permisiuni[6] = 'r';

  if (permisiuni[7] & S_IWUSR)
    permisiuni[7] = 'w';

  if (permisiuni[8] & S_IXUSR)
    permisiuni[8] = 'x';
}


void create_dir(char nume[20])
{
  if (mkdir(nume, 0750) == -1)
    {
      perror("Eroare la crearea directorului.\n");
    }
  else
    {
      printf("Directorul a fost creat cu succes.\n");
    }
  

  
}





int parse_condition(const char *input, char *field, char *op, char *value);


int match_condition(Report *r, const char *field, const char *op, const char *value);



int main(int argc, char **argv)
{
  if (argc < 7)
    {
      perror("Au fost furnizate prea multe argumente");
      exit(-1);
    }

  
  
  

  return 0;
}
