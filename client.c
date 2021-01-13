/* cliTCPIt.c - Exemplu de client TCP
   Trimite un numar la server; primeste de la server numarul incrementat.
         
   Autor: Lenuta Alboaie  <adria@infoiasi.ro> (c)2009
*/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>

/* codul de eroare returnat de anumite apeluri */
extern int errno;

/* portul de conectare la server*/
int port;

int main(int argc, char *argv[])
{
  int sd;                    // descriptorul de socket
  struct sockaddr_in server; // structura folosita pentru conectare

  port = 2908;

  /* cream socketul */
  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("Eroare la socket().\n");
    return errno;
  }

  /* umplem structura folosita pentru realizarea conexiunii cu serverul */
  /* familia socket-ului */
  server.sin_family = AF_INET;
  /* adresa IP a serverului */
  server.sin_addr.s_addr = inet_addr("127.0.0.1");
  /* portul de conectare */
  server.sin_port = htons(port);

  /* ne conectam la server */
  if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
  {
    perror("[client]Eroare la connect().\n");
    return errno;
  }
  char mesaj[1024], buf[1024];
  while (strcmp(mesaj, "quit") != 0)
  {
    printf("[client]Introduceti o comanda: ");
    fflush(stdout);
    scanf(" %s", buf);
    strcpy(mesaj, buf);

    printf("[client] Am citit %s\n", mesaj);

    if (write(sd, &mesaj, sizeof(mesaj)) <= 0)
    {
      perror("[client]Eroare la write() spre server.\n");
      return errno;
    }
    if (read(sd, &mesaj, sizeof(mesaj)) < 0)
    {
      perror("[client]Eroare la read() de la server.\n");
      return errno;
    }
    printf("[client]Mesajul primit este: %s\n", mesaj);
    
  }
  close(sd);
}
