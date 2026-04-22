#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


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



int parse_condition(const char *input, char *field, char *op, char *value);


int match_condition(Report *r, const char *field, const char *op, const char *value);



int main(int argc, char **argv)
{
  if (argc > 5)
    {
      perror("Au fost furnizate prea multe argumente");
      exit(-1);
    }

  
  
  

  return 0;
}
