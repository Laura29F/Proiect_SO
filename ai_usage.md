Tool-ul folosit este claude ai.

FUNCTIA 1 - int parse_condition(const char *input, char *field, char *op, char *value)

Prompt: Am nevoie de ajutorul tau pentru doua functii pe care sa le implementez intr-un proiect in C (materia Sisteme de Operare). Am o structura Report cu campurile: id (int), inspector (char[25]), coordonate (struct cu latitude și longitude float), issue_category (char[30]), severity (int, 1-3), time_stamp (time_t), description_text (char[500]). Scrie-mi o functie int parse_condition(const char *input, char *field, char *op, char *value) care imparte un string de forma 'field:operator:value' in cele trei parti. Operatorii posibili sunt: ==, !=, <, <=, >, >=."

Am primit o functie bazata pe strtok() care face o copie a input-ului si extrage field, op, value speratate prin ':'.

Modificari: Am adaugat terminatori de sir expliciti si dimensiuni pentru buffere deoarece varianta generata nu trunchia corect.

Am invatat ca strtok modifica sirul original prin inserarea de '\0', de aceea trebuie lucrat pe o copie, dar si diferenta dintre strtok(NULL, ":") și strtok(NULL, "") pentru ultimul token.


FUNCTIA 2 - int match_condition(Report *r, const char *field, const char *op, const char *value)

Prompot: Pentru acelasi proiect am nevoie si de o functie int match_condition(Report *r, const char *field, const char *op, const char *value) care returneaza 1 daca inregistrarea satisface conditia si 0 in caz contrar. Pentru severity si timestamp comparatiile sunt numerice, iar pentru category si inspector comparatiile sunt de tip string.

Am primit o functie cu if-uri pe strcmp(field, ...) care pentru severity si 
timestamp converteste value la tip numeric si compara, iar pentru category 
si inspector foloseste strcmp().

Modificari: AI-ul a omis operatorii <, <=, >, >= pentru campul timestamp,
i-am adaugat manual dupa ce am testat si am observat ca lipsesc. Am adaugat
si return 0 la final pentru field-uri necunoscute.

Am invatat ca AI-ul poate omite cazuri de margine si trebuie mereu testat,
dar si importanta conversiei corecte de tip: atoi pentru int, atol pentru 
time_t care poate fi long pe unele platforme.