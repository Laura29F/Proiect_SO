#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
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
      permisiuni[i] = '-';
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

int check_role_access(const char *path, const char *role, int need_read, int need_write)
{
  struct stat st;
  if (stat(path, &st) == -1)
    {
      perror("Eroare la citirea informatiilor despre fisier");
      return 0;
    }

  if (strcmp(role, "manager") == 0)
    {
      if (need_read && !(st.st_mode & S_IRUSR))
	{
	  fprintf(stderr, "Acces refuzat: managerul nu are drept de citire pe %s\n", path);
	  return 0;
	}
      if (need_write && !(st.st_mode & S_IWUSR))
	{
	  fprintf(stderr, "Acces refuzat: managerul nu are drept de scriere pe %s\n", path);
	  return 0;
	}
      return 1;
    }

  if (strcmp(role, "inspector") == 0)
    {
      if (need_read && !(st.st_mode & S_IRGRP))
	{
	  fprintf(stderr, "Acces refuzat: inspectorul nu are drept de citire pe %s\n", path);
	  return 0;
	}
      if (need_write && !(st.st_mode & S_IWGRP))
	{
	  fprintf(stderr, "Acces refuzat: inspectorul nu are drept de scriere pe %s\n", path);
	  return 0;
	}
      return 1;
    }
  fprintf(stderr, "Rol necunoscut: %s\n", role);
  return 0;
}

void log_action(const char *district, const char *role, const char *user, const char *action)
{
  char path[256];
  char line[512];
  int fd;
  int len;
  time_t now;
  now = time(NULL);

  snprintf(path, sizeof(path), "%s/logged_district", district);

 /* logged_district are permisiuni 644. In simularea pe roluri, managerul este owner si poate scrie, iar inspectorul nu are drept de scriere in log. */
  if (!check_role_access(path, role, 0, 1))
    {
      fprintf(stderr, "Actiunea nu a fost logata deoarece rolul '%s' nu are permisiuni de scriere.\n", role);
      return;
    }
  
  fd = open(path, O_WRONLY | O_APPEND);
  if (fd == -1)
    {
      perror("Eroare la deschiderea fisierului de log");
      exit(1);
    }

  len = snprintf(line, sizeof(line), "%ld\t%s\t%s\t%s\n", (long) now, role, user, action);

  if (write(fd, line, len) == -1)
    perror("Eroare la scrierea in fisierul de log");
  close(fd);
}


void update_symlink(const char *district)
{
  char link_name[256];
  char target[256];

  snprintf(link_name, sizeof(link_name), "active_reports-%s", district);
  snprintf(target, sizeof(target), "%s/reports.dat", district);

  unlink(link_name);

  if (symlink(target, link_name) == -1)
    {
      perror("Eroare la crearea symlink-ului");
      exit(1);
    }
}

void check_dangling_symlink(const char *district)
{
  char link_name[256];
  struct stat lst, st;

  snprintf(link_name, sizeof(link_name), "active_reports-%s", district);

  if (lstat(link_name, &lst) == -1) return;

  if (S_ISLNK(lst.st_mode))
    {
      if (stat(link_name, &st) == -1)
        {
          fprintf(stderr, "ATENTIE: symlink-ul '%s' este dangling!\n", link_name);
        }
    }
}

//intializeaza districtul
void create_district(const char *district)
{
  char path[256];
  int fd;

  if (mkdir(district, 0750) == -1)
    {
      if (errno == EEXIST)
	  printf("Directorul exista deja.\n");
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
  update_symlink(district);
  check_dangling_symlink(district);

  snprintf(path, sizeof(path), "%s/district.cfg", district);

  fd = open(path, O_CREAT | O_RDWR, 0640);
  if (fd == -1)
    {
      perror("Eroare la crearea sau deschiderea fisierului district.cfg");
      exit(1);
    }
  //daca fisierul e gol, scriem threshold-ul default
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

void add_report(const char *district, const char *role, const char *user)
{
  char path[256];
  int fd;
  Report r;
  int severity;

  create_district(district);
  snprintf(path, sizeof(path), "%s/reports.dat", district);

  if (!check_role_access(path, role, 1, 1))
    exit(1);
  
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
      printf("Eroare: valoarea introdusa nu face parte din cele permise.\n");
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
  log_action(district, role, user, "add");
  printf("Raportul %d a fost adaugat cu succes.\n", r.id);
}

void list_reports(const char *district, const char *role, const char *user)
{
  char path[256];
  int fd;
  Report r;
  ssize_t bytes_read;
  struct stat st;
  char permisiuni[10];

  snprintf(path, sizeof(path), "%s/reports.dat", district);
  if (!check_role_access(path, role, 1, 0))
    exit(1);
  
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

  log_action(district, role, user, "list");
  close(fd);
}

void view_report(const char *district, const char *role, const char *user, int report_id)
{
  char path[256];
  int fd;
  Report r;
  ssize_t bytes_read;
  int found = 0;

  snprintf(path, sizeof(path), "%s/reports.dat", district);
  if (!check_role_access(path, role, 1, 0))
    exit(1);
  
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
      printf("Nu exista raport cu ID-ul %d in districtul %s.\n", report_id, district);
  log_action(district, role, user, "view");
}

void remove_report(const char *district, const char *role, const char *user, int report_id)
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
  if (!check_role_access(path, role, 1, 1))
    exit(1);
  
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
  log_action(district, role, user, "remove_report");
  printf("Raportul cu ID-ul %d a fost sters cu succes.\n", report_id);  
}

void update_threshold(const char *district, const char *role, const char *user, int value)
{
  char path[256];
  int fd;
  struct stat st;
  mode_t perm;

  if (strcmp(role, "manager") != 0)
    {
      fprintf(stderr, "Eroare de permisiune: doar managerul poate modifica threshold-ul.\n");
      exit(1);
    }

  if (value < 1 || value > 3)
    {
      fprintf(stderr, "Eroare: threshold-ul trebuie sa fie 1, 2 sau 3.\n");
      exit(1);
    }
  snprintf(path, sizeof(path), "%s/district.cfg", district);

  if (stat(path, &st) == -1)
    {
      perror("Eroare la citirea informatiilor despre district.cfg");
      exit(1);
    }
  perm = st.st_mode & 0777;

  if (perm != 0640)
    {
      fprintf(stderr, "Eroare: district.cfg nu are permisiunile cerute 640.\n");
      fprintf(stderr, "Operatia update_threshold este refuzata.\n");
      exit(1);
    }

  if (!check_role_access(path, role, 1, 1))
    exit(1);

  fd = open(path, O_WRONLY | O_TRUNC);
  if (fd == -1)
    {
      perror("Eroare la deschiderea district.cfg pentru scriere");
      exit(1);
    }

  char line[64];
  int len = snprintf(line, sizeof(line), "threshold=%d\n", value);
  if (write(fd, line, len) != len)
    {
      perror("Eroare la scrierea noului threshold");
      close(fd);
      exit(1);
    }
  close(fd);
  log_action(district, role, user, "update_threshold");
  printf("Threshold-ul districtului %s a fost actualizat la %d.\n", district, value);
}

int parse_condition(const char *input, char *field, char *op, char *value)
{
  char copy[256];
  char *token;

  strncpy(copy, input, sizeof(copy) - 1);
  copy[sizeof(copy) - 1] = '\0';

  token = strtok(copy, ":");
  if (!token) return 0;
  strncpy(field, token, 31);
  field[31] = '\0';

  token = strtok(NULL, ":");
  if (!token) return 0;
  strncpy(op, token, 3);
  op[3] = '\0';

  token = strtok(NULL, "");
  if (!token) return 0;
  strncpy(value, token, 63);
  value[63] = '\0';

  return 1;
}

int match_condition(Report *r, const char *field, const char *op, const char *value)
{
  if (strcmp(field, "severity") == 0)
    {
      int val = atoi(value);
      if (strcmp(op, "==") == 0) return r->severity == val;
      if (strcmp(op, "!=") == 0) return r->severity != val;
      if (strcmp(op, "<")  == 0) return r->severity <  val;
      if (strcmp(op, "<=") == 0) return r->severity <= val;
      if (strcmp(op, ">")  == 0) return r->severity >  val;
      if (strcmp(op, ">=") == 0) return r->severity >= val;
    }
  else if (strcmp(field, "category") == 0)
    {
      int cmp = strcmp(r->issue_category, value);
      if (strcmp(op, "==") == 0) return cmp == 0;
      if (strcmp(op, "!=") == 0) return cmp != 0;
      if (strcmp(op, "<")  == 0) return cmp <  0;
      if (strcmp(op, "<=") == 0) return cmp <= 0;
      if (strcmp(op, ">")  == 0) return cmp >  0;
      if (strcmp(op, ">=") == 0) return cmp >= 0;
    }
  else if (strcmp(field, "inspector") == 0)
    {
      int cmp = strcmp(r->inspector, value);
      if (strcmp(op, "==") == 0) return cmp == 0;
      if (strcmp(op, "!=") == 0) return cmp != 0;
      if (strcmp(op, "<")  == 0) return cmp <  0;
      if (strcmp(op, "<=") == 0) return cmp <= 0;
      if (strcmp(op, ">")  == 0) return cmp >  0;
      if (strcmp(op, ">=") == 0) return cmp >= 0;
    }
  else if (strcmp(field, "timestamp") == 0)
    {
      time_t val = (time_t)atol(value);
      if (strcmp(op, "==") == 0) return r->time_stamp == val;
      if (strcmp(op, "!=") == 0) return r->time_stamp != val;
      if (strcmp(op, "<")  == 0) return r->time_stamp <  val;
      if (strcmp(op, "<=") == 0) return r->time_stamp <= val;
      if (strcmp(op, ">")  == 0) return r->time_stamp >  val;
      if (strcmp(op, ">=") == 0) return r->time_stamp >= val;
    }

  return 0;
}

void filter_reports(const char *district, const char *role, const char *user, int cond_count, char **conditions)
{
  char path[256];
  int fd;
  Report r;
  ssize_t bytes_read;
  int found = 0;

  snprintf(path, sizeof(path), "%s/reports.dat", district);
  if (!check_role_access(path, role, 1, 0))
    exit(1);
  fd = open(path, O_RDONLY);
  if (fd == -1)
    {
      perror("Eroare la deschiderea reports.dat pentru filter");
      exit(1);
    }

  while ((bytes_read = read(fd, &r, sizeof(Report))) == sizeof(Report))
    {
      int match = 1;

      for (int i = 0; i < cond_count; i++)
        {
          char field[32];
          char op[8];
          char value[64];

          if (!parse_condition(conditions[i], field, op, value))
            {
              fprintf(stderr, "Conditie invalida: %s\n", conditions[i]);
              close(fd);
              exit(1);
            }
          if (!match_condition(&r, field, op, value))
            {
              match = 0;
              break;
            }
        }

      if (match)
        {
          printf("ID: %d | Inspector: %s | Categorie: %s | Severitate: %d\n", r.id, r.inspector, r.issue_category, r.severity);
          printf("Coordonate: %.2f, %.2f | Timestamp: %s", r.coordonate.latitude, r.coordonate.longitude, ctime(&r.time_stamp));
          printf("Descriere: %s\n\n", r.description_text);
          found++;
        }
    }

  if (bytes_read == -1)
    {
      perror("Eroare la citirea din reports.dat pentru filter");
      close(fd);
      exit(1);
    }
  close(fd);
  if (found == 0)
    {
      printf("Niciun raport nu satisface conditiile date.\n");
    }
  log_action(district, role, user, "filter");
}

void remove_district(const char *district, const char *role)
{
  char link_name[256];
  pid_t pid;
  int status;

  if (strcmp(role, "manager") != 0)
    {
      fprintf(stderr, "Eroare de permisiune: doar managerul poate sterge un district.\n");
      exit(1);
    }

  if (district == NULL || strlen(district) == 0 || strcmp(district, ".") == 0 || strcmp(district, "..") == 0 || strchr(district, '/') != NULL)
    {
      fprintf(stderr, "Eroare: nume de district invalid pentru stergere.\n");
      exit(1);
    }
  snprintf(link_name, sizeof(link_name), "active_reports-%s", district);

  if (unlink(link_name) == -1)
    {
      if (errno != ENOENT)
        {
          perror("Eroare la stergerea symlink-ului districtului");
          exit(1);
        }
    }

  pid = fork();
  if (pid == -1)
    {
      perror("Eroare la fork");
      exit(1);
    }

  if (pid == 0)
    {
      execlp("rm", "rm", "-rf", district, (char *)NULL);

      perror("Eroare la execlp pentru rm -rf");
      exit(1);
    }

  if (waitpid(pid, &status, 0) == -1)
    {
      perror("Eroare la waitpid");
      exit(1);
    }

  if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
    {
      printf("Districtul %s a fost sters cu succes.\n", district);
    }
  else
    {
      fprintf(stderr, "Eroare: stergerea districtului %s nu s-a terminat cu succes.\n", district);
      exit(1);
    }
}

int main(int argc, char **argv)
{
  char *role = NULL;
  char *user = NULL;
  char *command = NULL;
  char *district = NULL;

  if (argc < 7)
    {
      fprintf(stderr, "Utilizare incorecta. Numar insuficient de argumente.\n");
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
      add_report(district, role, user);
    }
  else if (strcmp(command, "--list") == 0)
    {
      if (argc != 7)
        {
          fprintf(stderr, "Utilizare corecta: %s --role <rol> --user <nume> --list <district>\n", argv[0]);
          return 1;
        }
      district = argv[6];
      list_reports(district, role, user);
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
      view_report(district, role, user, report_id);
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
      remove_report(district, role, user, report_id);
    }
  else if (strcmp(command, "--update_threshold") == 0)
    {
      int value;
      if (argc != 8)
        {
          fprintf(stderr, "Utilizare corecta: %s --role manager --user <nume> --update_threshold <district> <value>\n", argv[0]);
          return 1;
        }
      if (strcmp(role, "manager") != 0)
        {
          fprintf(stderr, "Eroare de permisiune: doar rolul de 'manager' poate modifica threshold-ul.\n");
          return 1;
        }
      district = argv[6];
      value = atoi(argv[7]);

      if (value < 1 || value > 3)
        {
          fprintf(stderr, "Eroare: threshold-ul trebuie sa fie 1, 2 sau 3.\n");
          return 1;
        }
      update_threshold(district, role, user, value);
    }
  else if (strcmp(command, "--filter") == 0)
  {
    if (argc < 8)
      {
        fprintf(stderr, "Utilizare corecta: %s --role <rol> --user <nume> --filter <district> <conditie1> [conditie2 ...]\n", argv[0]);
        return 1;
      }

    district = argv[6];
    filter_reports(district, role, user, argc - 7, &argv[7]);
  }
  else if (strcmp(command, "--remove_district") == 0)
  {
    if (argc != 7)
      {
        fprintf(stderr, "Utilizare corecta: %s --role manager --user <nume> --remove_district <district>\n", argv[0]);
        return 1;
      }

    if (strcmp(role, "manager") != 0)
      {
        fprintf(stderr, "Eroare de permisiune: doar rolul de 'manager' poate sterge un district.\n");
        return 1;
      }

    district = argv[6];
    remove_district(district, role);
  }
  else
    {
      fprintf(stderr, "Eroare: comanda necunoscuta: %s\n", command);
      return 1;
    }
  check_dangling_symlink(district);
  
  return 0;
}
