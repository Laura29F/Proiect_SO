#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>


typedef struct{
  float latitude;
  float longitude;
} Coordinates;


typedef struct{

  int id;
  char inspector[25];
  Coordinates coordonate;
  char issue_category[30];
  int severity;
  time_t time_stamp;
  char description_text[500];
} Report;


void set_mode(mode_t mod, char permisiuni[10])
{
  int i;
  for (i = 0; i < 9; i++)
    {
      permisiuni[i] = '-';
    }
  permisiuni[9] = '\0';

  if (mod & S_IRUSR)
    permisiuni[0] = 'r';
  if (mod & S_IWUSR)
    permisiuni[1] = 'w';  
  if (mod & S_IXUSR)
    permisiuni[2] = 'x';
  
  if (mod & S_IRGRP)
    permisiuni[3] = 'r';
  if (mod & S_IWGRP)
    permisiuni[4] = 'w';
  if (mod & S_IXGRP)
    permisiuni[5] = 'x';
  
  if (mod & S_IROTH)
    permisiuni[6] = 'r';
  if (mod & S_IWOTH)
    permisiuni[7] = 'w';
  if (mod & S_IXOTH)
    permisiuni[8] = 'x';
}

//intializeaza districtul
void create_district(const char *district)
{
  char path[256];
  int fd;

  if (mkdir(district, 0750) == -1)
    {
      if (errno == EEXIST)
	{
	  printf("Directorul exista deja.\n");
	}
      else
	{
	  perror("Eroare la crearea directorului");
	  exit(1);
	}
    }
  else
    {
      printf("Directorul a fost creat.\n");
    }

  //ne asiguram ca directoarele au permisiunile cerute in enunt
  if (chmod(district, 0750) == -1)
    {
      perror("Eroare la setarea permisiunilor");
      exit(1);
    }

  snprintf(path, sizeof(path), "%s/reports.dat", district);

  fd = open(path, O_CREAT | O_RDWR | O_APPEND, 0664);
  if (fd == -1)
    {
      perror("Eroare la crearea sau deschiderea reports.dat");
      exit(1);
    }
  close(fd);

  if (chmod(path, 0664) == -1)
    {
      perror("Eroare la setare permisiunilor fisierului reports.dat");
      exit(1);
    }

  snprintf(path, sizeof(path), "%s/district.cfg", district);

  fd = open(path, O_CREAT | O_RDWR, 0640);
  if (fd == -1)
    {
      perror("Eroare la crearea sau deschiderea fisierului district.cfg");
      exit(1);
    }

  //dacca fisierul e gol, scriem threshold-ul default
  if (lseek(fd, 0, SEEK_END) == 0)
    {
      const char *default_cfg = "threshold=2\n";

      if (write(fd, default_cfg, strlen(default_cfg)) == -1)
	{
	  perror("Eroare la scriere in district.cfg");
	  close(fd);
	  exit(1);
	}
    }
  close(fd);

  if (chmod(path, 0640) == -1)
    {
      perror("Eroare la setarea permisiunilor pentru district.cfg");
      exit(1);
    }


  snprintf(path, sizeof(path), "%s/logged_district", district);

  fd = open(path, O_CREAT | O_RDWR | O_APPEND, 0644);
  if (fd == -1)
    {
      perror("Eroare la crearea sau deschiderea fisierului logged_district");
      exit(1);
    }
  close(fd);

  if (chmod(path, 0644) == -1)
    {
      perror("Eroare la setarea permisiunilor pentru logged_district");
      exit(1);
    }
  
  printf("Districtul a fost pregatit cu succes.\n");
}

//extrage urmatorul id disponibil
int get_next_report_id(const char *report_path)
{
  int fd;
  Report r;
  int max_id = 0;
  ssize_t bytes_read;

  fd = open(report_path, O_RDONLY);
  if (fd == -1)
    {
      perror("Eroare la deschiderea fisierului reports.dat pentru citire");
      exit(1);
    }
  
  //citeste secvential tot fisierul
  while ((bytes_read = read(fd, &r, sizeof(Report))) == sizeof(Report))
    {
      if (r.id > max_id)
	max_id = r.id;
    }

  if (bytes_read == -1)
    {
      perror("Eroare la citirea din reports.dat");
      close(fd);
      exit(1);
    }

  close(fd);

  return max_id + 1;
}


int valid_category(const char *category)
{
  if (strcmp(category, "road") == 0)
    return 1;

  if (strcmp(category, "lighting") == 0)
    return 1;

  if (strcmp(category, "flooding") == 0)
    return 1;

  return 0;
}

void add_report(const char *district, const char *user)
{
  char path[256];
  int fd;
  Report r;
  int severity;

  create_district(district);

  snprintf(path, sizeof(path), "%s/reports.dat", district);
      
  r.id = get_next_report_id(path);

  strncpy(r.inspector, user, sizeof(r.inspector) - 1);
  r.inspector[sizeof(r.inspector) - 1] = '\0';

  printf("Latitude: ");
  if (scanf("%f", &r.coordonate.latitude) != 1)
    {
      printf("Eroare la citirea latitudinii.\n");
      exit(1);
    }
  
  printf("Longitude: ");
  if (scanf("%f", &r.coordonate.longitude) != 1)
    {
      printf("Eroare la citirea longitudinii.\n");
      exit(1);    
    }

  printf("Category: ");
  if (scanf("%29s", r.issue_category) != 1 || !valid_category(r.issue_category))
    {
      printf("Eroare: categoria nu este valida.\n");
      exit(1);
    }

  printf("Severity (1/2/3): ");
  if (scanf("%d", &severity) != 1 || severity < 1 || severity > 3)
    {
      printf("Eroare: valoarea introdusa nu face parte din cele permise\n.");
      exit(1);
    }

  r.severity = severity;

  //curatarea buffer-ului
  int c;
  while ((c = getchar()) != '\n' && c != EOF)
    {

    }

  printf("Description: ");
  if (fgets(r.description_text, sizeof(r.description_text), stdin) == NULL)
    {
      printf("Eroare la citirea descrierii.\n");
      exit(1);
    }

  r.description_text[strcspn(r.description_text, "\n")] = '\0';

  if (strlen(r.description_text) == 0)
    {
      printf("Eroare: descrierea nu poate fi goala.\n");
      exit(1);
    }

  r.time_stamp = time(NULL);
  
  fd = open(path, O_WRONLY | O_APPEND);
  if (fd == -1)
    {
      perror("Eroare la deschiderea fisierului reports.dat pentru scriere");
      exit(1);
    }

  if (write(fd, &r, sizeof(Report)) != sizeof(Report))
    {
      perror("Eroare la scrierea raportului in reports.dat");
      close(fd);
      exit(1);
    }

  close(fd);
  printf("Raportul %d a fost adaugat cu succes.\n", r.id);
}

void list_reports(const char *district)
{
  char path[256];
  int fd;
  Report r;
  ssize_t bytes_read;
  struct stat st;
  char permisiuni[10];

  snprintf(path, sizeof(path), "%s/reports.dat", district);

  if (stat(path, &st) == -1)
    {
      perror("Eroare la citirea informatiilor despre reports.dat");
      exit(1);
    }

  set_mode(st.st_mode, permisiuni);

  printf("Informatii reports.dat:\n");
  printf("Permisiuni: %s\n", permisiuni);
  printf("Dimensiune: %ld bytes\n", st.st_size);
  printf("Ultima modificare: %s", ctime(&st.st_mtime));
  printf("\n");

  fd = open(path, O_RDONLY);
  if (fd == -1)
    {
      perror("Eroare la deschiderea fisierului reports.dat pentru citire");
      exit(1);
    }

  printf("Rapoarte din districtul %s:\n\n", district);

  while ((bytes_read = read(fd, &r, sizeof(Report))) == sizeof(Report))
    {
      printf("ID: %d\n", r.id);
      printf("Inspector: %s\n", r.inspector);
      printf("Coordonate: %.2f, %.2f\n", r.coordonate.latitude, r.coordonate.longitude);
      printf("Categorie: %s\n", r.issue_category);
      printf("Severitate: %d\n", r.severity);
      printf("Timestamp: %s", ctime(&r.time_stamp));
      printf("Descriere: %s\n", r.description_text);
      printf("\n");
    }

  if (bytes_read == -1)
    {
      perror("Eroare la citirea din reports.dat");
      close(fd);
      exit(1);
    }

  close(fd);
}

void view_report(const char *district, int report_id)
{
  char path[256];
  int fd;
  Report r;
  ssize_t bytes_read;
  int found = 0;

  snprintf(path, sizeof(path), "%s/reports.dat", district);

  fd = open(path, O_RDONLY);
  if (fd == -1)
    {
      perror("Eroare la deschiderea fisierului reports.dat pentru citire");
      exit(1);
    }

  while ((bytes_read = read(fd, &r, sizeof(Report))) == sizeof(Report))
    {
      if (r.id == report_id)
	{
	  printf("Raport gasit:\n");
	  printf("\n");
	  printf("ID: %d\n", r.id);
          printf("Inspector: %s\n", r.inspector);
          printf("Coordonate: %.2f, %.2f\n", r.coordonate.latitude, r.coordonate.longitude);
          printf("Categorie: %s\n", r.issue_category);
          printf("Severitate: %d\n", r.severity);
          printf("Timestamp: %s", ctime(&r.time_stamp));
          printf("Descriere: %s\n", r.description_text);
          printf("\n");

          found = 1;
          break;	  
	}
    }

  if (bytes_read == -1)
    {
      perror("Eroare la citirea din reports.dat");
      close(fd);
      exit(1);
    }

  close(fd);

  if (found == 0)
    {
      printf("Nu exista raport cu ID-ul %d in districtul %s.\n", report_id, district);
    }
}


void remove_report(const char *district, int report_id)
{
  char path[256];
  int fd;
  Report r;
  ssize_t bytes_read;
  off_t current_pos;
  off_t delete_pos = -1;
  off_t next_pos;
  off_t file_size;
  int found = 0;

  snprintf(path, sizeof(path), "%s/reports.dat", district);
  
  fd = open(path, O_RDWR);
  if (fd == -1)
    {
      perror("Eroare la deschiderea fisierului reports.dat pentru stergere");
      exit(1);
    }

  while ((bytes_read = read(fd, &r, sizeof(Report))) == sizeof(Report))
    {
      if (r.id == report_id)
	{
	  current_pos = lseek(fd, 0, SEEK_CUR);
	  delete_pos = current_pos - sizeof(Report);
	  found = 1;
	  break;
	}
    }

  if (bytes_read == -1)
    {
      perror("Eroare la citirea din reports.dat");
      close(fd);
      exit(1);
    }

  if (found == 0)
    {
      printf("Nu exista raport cu ID-ul %d.\n", report_id);
      close(fd);
      return;
    }

  file_size = lseek(fd, 0, SEEK_END);
  if (file_size == -1)
    {
      perror("Eroare la aflarea dimensiunii fisierului");
      close(fd);
      exit(1);
    }

  next_pos = delete_pos + sizeof(Report);

  while (next_pos < file_size)
    {
      if (lseek(fd, next_pos, SEEK_SET) == -1)
	{
	  perror("Eroare la lseek pentru citire");
	  close(fd);
	  exit(1);
	}
      
      if (read(fd, &r, sizeof(Report)) != sizeof(Report))
	{
	  perror("Eroare la citirea raportului care trebuie mutat");
	  close(fd);
	  exit(1);
	}

      if (lseek(fd, next_pos - sizeof(Report), SEEK_SET) == -1)
        {
          perror("Eroare la lseek pentru scriere");
          close(fd);
          exit(1);
        }

      if (write(fd, &r, sizeof(Report)) != sizeof(Report))
        {
          perror("Eroare la mutarea raportului");
          close(fd);
          exit(1);
        }

      next_pos = next_pos + sizeof(Report);
    }

  if (ftruncate(fd, file_size - sizeof(Report)) == -1)
    {
      perror("Eroare la ftruncate");
      close(fd);
      exit(1);
    }

  close(fd);

  printf("Raportul cu ID-ul %d a fost sters cu succes.\n", report_id);
  
}


int parse_condition(const char *input, char *field, char *op, char *value);


int match_condition(Report *r, const char *field, const char *op, const char *value);



int main(int argc, char **argv)
{
  char *role = NULL;
  char *user = NULL;
  char *command = NULL;
  char *district = NULL;

  if (argc < 7)
    {
      fprintf(stderr, "Utilizare incorecta. Numar insuficient de argumente.\n");
      fprintf(stderr, "Ex: %s --role <inspector|manager> --user <nume> --add <district>\n", argv[0]);
      return 1;
    }

  if (strcmp(argv[1], "--role") != 0)
    {
      fprintf(stderr, "Eroare: primul argument trebuie sa fie '--role'.\n");
      return 1;
    }

  role = argv[2];
  if (strcmp(role, "inspector") != 0 && strcmp(role, "manager") != 0)
    {
      fprintf(stderr, "Eroare: rolul trebuie sa fie strict 'inspector' sau 'manager'.\n");
      return 1;
    }

  if (strcmp(argv[3], "--user") != 0)
    {
      fprintf(stderr, "Eroare: al treilea argument trebuie sa fie '--user'.\n");
      return 1;
    }
  user = argv[4];
  command = argv[5];

  if (strcmp(command, "--add") == 0)
    {
      if (argc != 7)
        {
          fprintf(stderr, "Utilizare corecta: %s --role <rol> --user <nume> --add <district>\n", argv[0]);
          return 1;
        }
      district = argv[6];
      add_report(district, user);
    }
  else if (strcmp(command, "--list") == 0)
    {
      if (argc != 7)
        {
          fprintf(stderr, "Utilizare corecta: %s --role <rol> --user <nume> --list <district>\n", argv[0]);
          return 1;
        }
      district = argv[6];
      list_reports(district);
    }
  else if (strcmp(command, "--view") == 0)
    {
      int report_id;
      if (argc != 8)
        {
          fprintf(stderr, "Utilizare corecta: %s --role <rol> --user <nume> --view <district> <report_id>\n", argv[0]);
          return 1;
        }
      district = argv[6];
      report_id = atoi(argv[7]);

      if (report_id <= 0)
        {
          fprintf(stderr, "Eroare: report_id trebuie sa fie un numar pozitiv.\n");
          return 1;
        }
      view_report(district, report_id);
    }
  else if (strcmp(command, "--remove_report") == 0)
    {
      int report_id;
      if (argc != 8)
        {
          fprintf(stderr, "Utilizare corecta: %s --role manager --user <nume> --remove_report <district> <id>\n", argv[0]);
          return 1;
        }

      if (strcmp(role, "manager") != 0)
        {
          fprintf(stderr, "Eroare de permisiune: doar rolul de 'manager' poate sterge rapoarte.\n");
          return 1;
        }

      district = argv[6];
      report_id = atoi(argv[7]);

      if (report_id <= 0)
        {
          fprintf(stderr, "Eroare: report_id trebuie sa fie un numar pozitiv.\n");
          return 1;
        }
      remove_report(district, report_id);
    }
  else
    {
      fprintf(stderr, "Eroare: comanda necunoscuta: %s\n", command);
      return 1;
    }

  return 0;
}
