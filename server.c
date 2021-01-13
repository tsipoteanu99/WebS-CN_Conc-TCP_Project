/* servTCPConcTh2.c - Exemplu de server TCP concurent care deserveste clientii
   prin crearea unui thread pentru fiecare client.
   Asteapta un numar de la clienti si intoarce clientilor numarul incrementat.
	Intoarce corect identificatorul din program al thread-ului.
  
   
   Autor: Lenuta Alboaie  <adria@infoiasi.ro> (c)2009
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <sqlite3.h>
#include <dirent.h>
#define PORT 2908

/* codul de eroare returnat de anumite apeluri */
extern int errno;
int globalLogin = 0;
char globalUser[1024];
char globalFiles[2048] = "";

typedef struct thData
{
  int idThread; //id-ul thread-ului tinut in evidenta de acest program
  int cl;       //descriptorul intors de accept
} thData;

static void *treat(void *);
void raspunde(void *);

static int iterate_users(void *data, int argc, char **argv, char **azColName)
{
  int i;

  for (int i = 0; i < argc; i++)
  {
    if (strcmp(argv[i], globalUser) == 0)
    {
      printf("te-ai logat \n");
      globalLogin = 1;
    }
  }
  printf("\n");
  return 0;
}

int login(char *username)
{
  sqlite3 *database;
  int opendb = sqlite3_open("database.db", &database);
  char *query = "SELECT * FROM USERS;";
  char *somearg;
  strcpy(globalUser, username);
  if (opendb)
  {
    printf("Error when open database");
  }

  if (sqlite3_exec(database, query, iterate_users, (void *)somearg, NULL) != SQLITE_OK)
    printf("Error on runing query\n");

  sqlite3_close(database);
  if (globalLogin)
  {
    globalLogin = 0;
    return 1;
  }
  return 0;
}

int my_register(char *username)
{
  if (login(username))
    return 0;
  sqlite3 *database;
  int opendb = sqlite3_open("database.db", &database);
  char query[1024];
  sprintf(query, "INSERT INTO USERS (NUME) VALUES ('%s');", username);
  char *somearg;
  strcpy(globalUser, username);
  if (opendb)
  {
    printf("Error when open database");
  }
  int run_query;
  if (run_query = sqlite3_exec(database, query, NULL, 0, NULL) != SQLITE_OK)
    printf("Error on runing query\n");

  sqlite3_close(database);
  if (run_query != SQLITE_OK)
    return 0;
  else
    return 1;
}

int addFile(char *localpath, char *filename)
{
  char aux;
  FILE *local, *server;

  if (access(localpath, F_OK) == -1)
  {
    printf("Fisierul nu exista!");
    return -1;
  }
  else
  {
    local = fopen(localpath, "r");
    if (local == NULL)
    {
      printf("Eroare la deschidere fisier local");
      return 0;
    }
  }

  if (access(filename, F_OK) != -1)
  {
    printf("Fisierul deja exista!");
    return -2;
  }
  else
  {
    server = fopen(filename, "w");
    if (local == NULL)
    {
      fclose(local);
      printf("Eroare la creare fisier server");
      return 0;
    }
  }

  while ((aux = fgetc(local)) != EOF)
    fputc(aux, server);

  fclose(local);
  fclose(server);
  return 1;
}

int getFile(char *filename, char *localpath)
{
  char aux;
  FILE *local, *server;

  if (access(filename, F_OK) == -1)
  {
    printf("Fisierul nu exista pe server!");
    return -1;
  }
  else
  {
    server = fopen(filename, "r");
    if (server == NULL)
    {
      printf("Eroare la deschidere fisier server");
      return 0;
    }
  }

  if (access(localpath, F_OK) != -1)
  {
    printf("Fisierul deja exista!");
    return -2;
  }
  else
  {
    local = fopen(filename, "w");
    if (local == NULL)
    {
      fclose(local);
      printf("Eroare la creare fisier local");
      return 0;
    }
  }

  while ((aux = fgetc(server)) != EOF)
    fputc(aux, local);

  fclose(local);
  fclose(server);
  return 1;
}

int listFiles()
{
  strcpy(globalFiles, "");
  DIR *dp;
  struct dirent *dirent;
  dp = opendir(".");
  if (dp != NULL)
  {
    while ((dirent = readdir(dp)) != NULL)
    {
      if (strstr(dirent->d_name, ".html"))
      {
        strcat(globalFiles, "\n");
        strcat(globalFiles, dirent->d_name);
      }
    }
  }
  else
  {
    closedir(dp);
    return 0;
  }
  closedir(dp);
  if (strlen(globalFiles) > 1)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

int deleteFile(char *filename)
{
  if (remove(filename))
  {
    printf("File was not deleted!");
    return 0;
  }
  else
  {
    printf("File was deleted succesfully!");
    return 1;
  }
}

int main()
{
  struct sockaddr_in server; // structura folosita de server
  struct sockaddr_in from;
  int sd; //descriptorul de socket
  int pid;
  pthread_t th[100]; //Identificatorii thread-urilor care se vor crea
  int i = 0;

  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("[server]Eroare la socket().\n");
    return errno;
  }
  int on = 1;
  setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

  /* pregatirea structurilor de date */
  bzero(&server, sizeof(server));
  bzero(&from, sizeof(from));

  /* umplem structura folosita de server */
  /* stabilirea familiei de socket-uri */
  server.sin_family = AF_INET;
  /* acceptam orice adresa */
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  server.sin_port = htons(PORT);

  /* atasam socketul */
  if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
  {
    perror("[server]Eroare la bind().\n");
    return errno;
  }

  /* punem serverul sa asculte daca vin clienti sa se conecteze */
  if (listen(sd, 2) == -1)
  {
    perror("[server]Eroare la listen().\n");
    return errno;
  }
  /* servim in mod concurent clientii...folosind thread-uri */
  while (1)
  {
    int client;
    thData *td; //parametru functia executata de thread
    int length = sizeof(from);

    printf("[server]Asteptam la portul %d...\n", PORT);
    fflush(stdout);

    /* acceptam un client (stare blocanta pina la realizarea conexiunii) */
    if ((client = accept(sd, (struct sockaddr *)&from, &length)) < 0)
    {
      perror("[server]Eroare la accept().\n");
      continue;
    }

    /* s-a realizat conexiunea, se astepta mesajul */

    // int idThread; //id-ul threadului
    // int cl; //descriptorul intors de accept

    td = (struct thData *)malloc(sizeof(struct thData));
    td->idThread = i++;
    td->cl = client;

    pthread_create(&th[i], NULL, &treat, td);

  }
};
static void *treat(void *arg)
{
  struct thData tdL;
  tdL = *((struct thData *)arg);
  printf("[thread]- %d - Asteptam mesajul...\n", tdL.idThread);
  fflush(stdout);
  pthread_detach(pthread_self());
  raspunde((struct thData *)arg);
  close((intptr_t)arg);
  return (NULL);
};

void raspunde(void *arg)
{
  char mesaj[1024];
  int loggedIn = 0;
  struct thData tdL;
  tdL = *((struct thData *)arg);
  while (read(tdL.cl, &mesaj, sizeof(mesaj)) > 0)
  {
    int validCmd = 0;
    printf("[Thread %d]Mesajul a fost receptionat...%s\n", tdL.idThread, mesaj);
    printf("[Thread %d]Trimitem mesajul inapoi...%s\n", tdL.idThread, mesaj);

    //------------------------------------------- LOGIN -----------------------------------------

    if (strcmp(mesaj, "login") == 0)
    {
      if (loggedIn == 0)
      {
        validCmd = 1;
        if (write(tdL.cl, "Introdu un username:\n", 23) <= 0)
        {
          printf("[Thread %d] ", tdL.idThread);
          perror("[Thread]Eroare la write() catre client.\n");
        }
        else
          printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
        if (read(tdL.cl, &mesaj, sizeof(mesaj)) <= 0)
        {
          printf("[Thread %d] \n", tdL.idThread);
          perror("[Thread %d]Eroare la read() de catre client. \n");
        }
        else
        {
          if (login(mesaj) == 1)
          {
            if (write(tdL.cl, "Te-ai logat cu succes!\n", 25) <= 0)
            {
              printf("[Thread %d] ", tdL.idThread);
              perror("[Thread]Eroare la write() catre client.\n");
            }
            else
              printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
            loggedIn = 1;
          }
          else
          {
            if (write(tdL.cl, "Nu s-a putut face logarea...\n", 31) <= 0)
            {
              printf("[Thread %d] ", tdL.idThread);
              perror("[Thread]Eroare la write() catre client.\n");
            }
            else
              printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
          }
        }
      }
      else
      {
        if (write(tdL.cl, "Esti deja logat pe un cont!\n", 30) <= 0)
        {
          printf("[Thread %d] ", tdL.idThread);
          perror("[Thread]Eroare la write() catre client.\n");
        }
        else
          printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
      }
    }
    else if (strcmp(mesaj, "logout") == 0)
    {
      validCmd = 1;
      if (write(tdL.cl, "Te-ai delogat cu succes!\n", 27) <= 0)
      {
        printf("[Thread %d] ", tdL.idThread);
        perror("[Thread]Eroare la write() catre client.\n");
      }
      else
        printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
      loggedIn = 0;
    }

    //------------------------------ REGISTER -----------------------------------------

    else if (strcmp(mesaj, "register") == 0)
    {
      validCmd = 1;
      if (write(tdL.cl, "Introdu un username:\n", 23) <= 0)
      {
        printf("[Thread %d] ", tdL.idThread);
        perror("[Thread]Eroare la write() catre client.\n");
      }
      else
        printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
      if (read(tdL.cl, &mesaj, sizeof(mesaj)) <= 0)
      {
        printf("[Thread %d] \n", tdL.idThread);
        perror("[Thread %d]Eroare la read() de catre client. \n");
      }
      else
      {
        if (my_register(mesaj))
        {
          if (write(tdL.cl, "Te-ai inregistrat cu succes!\n", 30) <= 0)
          {
            printf("[Thread %d] ", tdL.idThread);
            perror("[Thread]Eroare la write() catre client.\n");
          }
          else
            printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
        }
        else
        {
          if (write(tdL.cl, "Nu te-ai putut inregistra...\n", 31) <= 0)
          {
            printf("[Thread %d] ", tdL.idThread);
            perror("[Thread]Eroare la write() catre client.\n");
          }
          else
            printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
        }
      }
    }

    //----------------------------------------------- GET --------------------------------------

    else if (strcmp(mesaj, "get") == 0)
    {
      if (loggedIn)
      {
        validCmd = 1;
        char path[1024], filename[1024];
        if (write(tdL.cl, "Introdu numele fisierului pe care vrei sa il descarci:\n", 57) <= 0)
        {
          printf("[Thread %d] ", tdL.idThread);
          perror("[Thread]Eroare la write() catre client.\n");
        }
        else
          printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
        if (read(tdL.cl, &mesaj, sizeof(mesaj)) <= 0)
        {
          printf("[Thread %d] \n", tdL.idThread);
          perror("[Thread %d]Eroare la read() de catre client. \n");
        }
        else
        {
          strcpy(filename, mesaj);
          if (write(tdL.cl, "Introdu path-ul la care va fi salvat fisierul:\n", 49) <= 0)
          {
            printf("[Thread %d] ", tdL.idThread);
            perror("[Thread]Eroare la write() catre client.\n");
          }
          else
            printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
          if (read(tdL.cl, &mesaj, sizeof(mesaj)) <= 0)
          {
            printf("[Thread %d] \n", tdL.idThread);
            perror("[Thread %d]Eroare la read() de catre client. \n");
          }
          else
          {
            strcpy(path, mesaj);
            if (addFile(filename, path) == 1)
            {
              if (write(tdL.cl, "Fisierul a fost descarcat cu succes!\n", 39) <= 0)
              {
                printf("[Thread %d] ", tdL.idThread);
                perror("[Thread]Eroare la write() catre client.\n");
              }
              else
                printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
            }
            else if (write(tdL.cl, "Fisierul nu a fost descarcat...\n", 34) <= 0)
            {
              printf("[Thread %d] ", tdL.idThread);
              perror("[Thread]Eroare la write() catre client.\n");
            }
            else
              printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
          }
        }
      }
      else
      {
        if (write(tdL.cl, "Trebuie sa te loghezi ca sa ai acces la aceasta comanda!\n", 59) <= 0)
        {
          printf("[Thread %d] ", tdL.idThread);
          perror("[Thread]Eroare la write() catre client.\n");
        }
        else
          printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
      }
    }

    //-------------------------------------- ADD ------------------------------------

    else if (strcmp(mesaj, "add") == 0)
    {
      if (loggedIn)
      {
        validCmd = 1;
        char path[1024], filename[1024];
        if (write(tdL.cl, "Introdu calea exacta catre fisierul local:\n", 45) <= 0)
        {
          printf("[Thread %d] ", tdL.idThread);
          perror("[Thread]Eroare la write() catre client.\n");
        }
        else
          printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
        if (read(tdL.cl, &mesaj, sizeof(mesaj)) <= 0)
        {
          printf("[Thread %d] \n", tdL.idThread);
          perror("[Thread %d]Eroare la read() de catre client. \n");
        }
        else
        {
          strcpy(path, mesaj);
          if (write(tdL.cl, "Introdu numele noului fisier:\n", 32) <= 0)
          {
            printf("[Thread %d] ", tdL.idThread);
            perror("[Thread]Eroare la write() catre client.\n");
          }
          else
            printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
          if (read(tdL.cl, &mesaj, sizeof(mesaj)) <= 0)
          {
            printf("[Thread %d] \n", tdL.idThread);
            perror("[Thread %d]Eroare la read() de catre client. \n");
          }
          else
          {
            strcpy(filename, mesaj);
            if (addFile(path, filename) == 1)
            {
              if (write(tdL.cl, "Fisierul a fost incarcat cu succes!\n", 38) <= 0)
              {
                printf("[Thread %d] ", tdL.idThread);
                perror("[Thread]Eroare la write() catre client.\n");
              }
              else
                printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
            }
            else if (write(tdL.cl, "Fisierul nu a fost incarcat...\n", 32) <= 0)
            {
              printf("[Thread %d] ", tdL.idThread);
              perror("[Thread]Eroare la write() catre client.\n");
            }
            else
              printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
          }
        }
      }
      else
      {
        if (write(tdL.cl, "Trebuie sa te loghezi ca sa ai acces la aceasta comanda!\n", 59) <= 0)
        {
          printf("[Thread %d] ", tdL.idThread);
          perror("[Thread]Eroare la write() catre client.\n");
        }
        else
          printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
      }
    }

    //----------------------------------------------- LIST ----------------------------------------------

    else if (strcmp(mesaj, "list") == 0)
    {
      if (loggedIn)
      {
        validCmd = 1;
        if (listFiles())
        {
          if (write(tdL.cl, &globalFiles, strlen(globalFiles) + 1) <= 0)
          {
            printf("[Thread %d] ", tdL.idThread);
            perror("[Thread]Eroare la write() catre client.\n");
          }
          else
            printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
        }
        else
        {
          if (write(tdL.cl, "Serverul nu contine fisiere html!\n", 36) <= 0)
          {
            printf("[Thread %d] ", tdL.idThread);
            perror("[Thread]Eroare la write() catre client.\n");
          }
          else
            printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
        }
      }
      else
      {
        if (write(tdL.cl, "Trebuie sa te loghezi ca sa ai acces la aceasta comanda!\n", 59) <= 0)
        {
          printf("[Thread %d] ", tdL.idThread);
          perror("[Thread]Eroare la write() catre client.\n");
        }
        else
          printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
      }
    }

    //-------------------------------------- DELETE -----------------------------------------

    else if (strcmp(mesaj, "delete") == 0)
    {
      if (loggedIn)
      {
        validCmd = 1;
        if (write(tdL.cl, "Ce fisier doresti sa stergi?\n", 31) <= 0)
        {
          printf("[Thread %d] ", tdL.idThread);
          perror("[Thread]Eroare la write() catre client.\n");
        }
        else
          printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
        if (read(tdL.cl, &mesaj, sizeof(mesaj)) <= 0)
        {
          printf("[Thread %d] \n", tdL.idThread);
          perror("[Thread %d]Eroare la read() de catre client. \n");
        }
        else
        {
          if (deleteFile(mesaj))
          {
            if (write(tdL.cl, "Fisierul a fost sters cu succes!\n", 35) <= 0)
            {
              printf("[Thread %d] ", tdL.idThread);
              perror("[Thread]Eroare la write() catre client.\n");
            }
            else
              printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
          }
          else
          {
            if (write(tdL.cl, "Nu s-a putut sterge fisierul!\n", 31) <= 0)
            {
              printf("[Thread %d] ", tdL.idThread);
              perror("[Thread]Eroare la write() catre client.\n");
            }
            else
              printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
          }
        }
      }
      else
      {
        if (write(tdL.cl, "Trebuie sa te loghezi ca sa ai acces la aceasta comanda!\n", 59) <= 0)
        {
          printf("[Thread %d] ", tdL.idThread);
          perror("[Thread]Eroare la write() catre client.\n");
        }
        else
          printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
      }
    }

    //----------------------------------------------- QUIT -------------------------------------------

    else if (strcmp(mesaj, "quit") == 0)
    {
      validCmd = 1;
      if (write(tdL.cl, "quit", 6) <= 0)
      {
        printf("[Thread %d] ", tdL.idThread);
        perror("[Thread]Eroare la write() catre client.\n");
      }
      else
        printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
    }

    //------------------------------------ CHECK IF VALID COMMAND ----------------------------------------

    else if (validCmd == 0)
    {
      if (write(tdL.cl, "Introdu o comanda valida!\n", 28) <= 0)
      {
        printf("[Thread %d] ", tdL.idThread);
        perror("[Thread]Eroare la write() catre client.\n");
      }
      else
        printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
    }
  }
}