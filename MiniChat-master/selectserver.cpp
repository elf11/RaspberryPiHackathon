#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Messages.h"

#define MAX_CLIENTS	20
#define BUFLEN 256

using namespace std;

void error(const char *msg)
{
    perror(msg);
    exit(1);
}


//adauga un client in lista, daca nu exista
int add_cli(int id, vector<client> &c, int port, char* nume, char* addr) {

	int i;
	client cl;

	for (i = 0; i < c.size(); i++)
		if (strcmp(nume, c[i].nume) == 0)
			return 1;
	cl.nume = strdup(nume);
	cl.addr = strdup(addr);
	cl.id = id;
	cl.port = port;
	cl.c_time = time(NULL);
	c.push_back(cl);
	return 0;
}

//se sterge un client din list
void rem_cli (int id, vector<client> &c) {

	int i, j;
	for (i = 0; i < c.size(); i++)
		if (c[i].id == id) {
			printf("Clientul %s s-a deconectat.\n", c[i].nume);
			free(c[i].nume);
			free(c[i].addr);
			for (j = 0; j < c[i].fisiere.size(); j++)	
				free(c[i].fisiere[j]);
			c.erase(c.begin() + i);
			break;
		}
}

//afiseaza clientii existenti
void afi_cli(vector<client> &c) {

	int i, j;
	for (i = 0; i < c.size(); i++) {
		printf ("Nume: %s\nAdresa: %s\nPort: %i\nLista fisiere: ", c[i].nume, c[i].addr, c[i].port);
		for(j = 0; j < c[i].fisiere.size(); j++)
			printf("%s ", c[i].fisiere[j]);
	printf("\n");
	}
}

//se intoarce un mesaj cu toti clientii
msg get_cli (vector<client> &c) {

	msg m;	
	int i;
	memset(m.payload, 0, 1024);
	for (i = 0;  i < c.size(); i++) {
		strcat(m.payload, c[i].nume);
		strcat(m.payload, " ");
	}

	m.type = 1;
	return m;
}

//se intoarce un mesaj cu informatii despre un client
msg get_cli (vector<client> &c, char* cli) {

	msg m;	
	int i;
	memset(m.payload, 0, 1024);
	for (i = 0;  i < c.size(); i++) 
		if (strcmp(cli, c[i].nume) == 0) {
			sprintf(m.payload, "Nume: %s\nPort: %i\nTimp conectare: %li",
					 c[i].nume, c[i].port, time(NULL) - c[i].c_time);
			break;
		}
	
	m.info = 0;
	if (i == c.size()) {
		m.info = 1;
		strcpy(m.payload, cli);
	}
	m.type = 2;
	return m;
}

//se intoarce un mesaj cu informatii despre adresa si portul unui client
msg get_cli_addr(vector<client> &c, char* cli) {

	msg m;	
	int i;
	memset(m.payload, 0, 1024);
	for (i = 0;  i < c.size(); i++) 
		if (strcmp(cli, c[i].nume) == 0) {
			sprintf(m.payload, "%s %i %s", c[i].nume, c[i].port, c[i].addr);
			break;
		}

	m.info = 0;
	if (i == c.size()) {
		m.info = 1;
		strcpy(m.payload, cli);
	}
	m.type = 3;
	return m;
}

//se intoarce un mesaj cu informatii despre adresa si portul unui client
msg get_cli_addr_f(vector<client> &c, char* cli) {

	msg m;
	char nume[50], fisier[50];	
	int i, j;
	memset(m.payload, 0, 1024);
	sscanf(cli, "%s%s", nume, fisier);

	for (i = 0;  i < c.size(); i++) 
		if (strcmp(nume, c[i].nume) == 0) {
			for (j = 0; j < c[i].fisiere.size(); j++)
				if (strcmp(fisier, c[i].fisiere[j]) == 0) {
					sprintf(m.payload, "%s %s %i %s", fisier, c[i].nume, c[i].port, c[i].addr);
					break;
				}
			break;
		}

	m.info = 0;
	if (i == c.size()) {
		m.info = 1;
		strcpy(m.payload, nume);
	}
	else
		if (j == c[i].fisiere.size()) {
			m.info = 2;
			strcpy(m.payload, cli);
		}
			
	m.type = 7;
	return m;
}

//se intoarce un mesaj cu informatii despre fisierele unui client
msg get_share (vector<client> &c, char* cli) {

	msg m;	
	int i, j;
	memset(m.payload, 0, 1024);
	strcpy(m.payload, cli);
	for (i = 0;  i < c.size(); i++) 
		if (strcmp(cli, c[i].nume) == 0) {
			for(j = 0; j < c[i].fisiere.size(); j++) {
				strcat(m.payload, " ");
				strcat(m.payload, c[i].fisiere[j]);
			}
			break;
		}

	m.info = 0;
	if (i == c.size())
		m.info = 1;
	m.type = 6;
	return m;
}
		

//aduaga un fisier la lista cu fisiere a unui client, daca nu exista deja
void add_file(int id, vector<client> &c, char* fisier){
	int i, j;
	for (int i = 0; i < c.size(); i++) {
		if (c[i].id == id) {
			for (j = 0; j < c[i].fisiere.size(); j++)
				if (strcmp (fisier, c[i].fisiere[j]) == 0)
					return;
			//se adauga fisierul daca nu a fost gasit
			c[i].fisiere.push_back(strdup(fisier));
		}
	}
}

//sterge un fisier din lista cu fisiere a unui client, daca exista
void remove_file(int id, vector<client> &c, char* fisier){
	int i, j;
	for (int i = 0; i < c.size(); i++) {
		if (c[i].id == id) {
			for (j = 0; j < c[i].fisiere.size(); j++)
				// se cauta fisierul si se sterge
				if (strcmp (fisier, c[i].fisiere[j]) == 0) {
					free(c[i].fisiere[j]);
					c[i].fisiere.erase(c[i].fisiere.begin() + j);
				}
		}
	}
}

int main(int argc, char *argv[])
{
	int sockfd, newsockfd, portno, cont = 1;
	unsigned int clilen;
	char buffer[BUFLEN];
	struct sockaddr_in serv_addr, cli_addr;
	int n, i, j;
	vector <client> clienti;
	msg mesaj;
	client aux_c;

	fd_set read_fds;	
	fd_set tmp_fds;	 
	int fdmax;		

	if (argc < 2) {
		fprintf(stderr,"Usage : %s port\n", argv[0]);
		exit(1);
	}

	//golim multimea de descriptori de citire (read_fds) si multimea tmp_fds 
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);
     
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
		error("Nu s-a putut deschide soketul");
     
	portno = atoi(argv[1]);

	//se face bind pe soketul de ascultare
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
     
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0) 
		error("Eroare la bind");
     
	listen(sockfd, MAX_CLIENTS);

	//adaugam soketul pe care se asculta conexiuni si stdin in multimea read_fds
	FD_SET(0, &read_fds);
	FD_SET(sockfd, &read_fds);
	fdmax = sockfd;

	// main loop
	while (cont) {
		tmp_fds = read_fds; 
		if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1) 
			error("Eroare la select");
	
		for(i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				
				//s-a primit o comanda
				if (i == 0){
					scanf("%s", buffer);
					if (strcmp(buffer, "quit") == 0) {
						cont = 0;
						//se inchid conexiunile in caz de iesire
						for (i = 1; i <= fdmax; i++)
							if (FD_ISSET(i, &read_fds))
								close(i);
						continue;
					}

					if (strcmp(buffer, "status") == 0)
						afi_cli(clienti); 
					else
						printf("Comanda gresita\n");

					continue;
					
					}
			
				if (i == sockfd) {
					// a venit ceva pe socketul de ascultare = o noua conexiune
					// actiunea serverului: accept()
					clilen = sizeof(cli_addr);
					if ((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen)) == -1) {
						error("Eroare in accept");
					} 
					else {
						//se seteaza noua conexiune
						FD_SET(newsockfd, &read_fds);
						if (newsockfd > fdmax) { 
							fdmax = newsockfd;
						}

						if ((n = recv(newsockfd, &mesaj, sizeof(mesaj), 0)) <= 0)
							error("Eroare in recv");
						
						//se verifica existenta clientului si se trimite confirmarea sau respingerea conexiunii
						sscanf(mesaj.payload, "%i%s", &n, buffer);
						if (add_cli(newsockfd, clienti, n, buffer, inet_ntoa(cli_addr.sin_addr))) {
							mesaj.type = -1;
							sprintf(mesaj.payload, "exista deja un client cu numele %s", buffer);
						}
						else
							printf("Noua conexiune cu clientul %s\n", buffer);
						send(newsockfd, &mesaj, sizeof(mesaj), 0);

					}
				}
					
				else {
					// se primesc date pe unul din socketii clientilor
					if ((n = recv(i, &mesaj, sizeof(mesaj), 0)) <= 0) {
						if (n < 0) 
							error("Eroare in recv");
					
						//se scoate clientul din lista in caz ca se inchide conexiunea
						rem_cli(i, clienti);
						//se inchide soketul si se scoate din multime
						close(i);
						FD_CLR(i, &read_fds);
					} 
					
					else { //s-a primit un mesaj care este tratat corespunzator
						switch (mesaj.type) {

							//se intoarce o lista cu toti clientii
							case 1:
								mesaj = get_cli(clienti);
								send(i, &mesaj, sizeof(mesaj), 0);
								break;
							
							//se intorc informatii despre un client
							case 2:
								mesaj = get_cli(clienti, mesaj.payload);
								send(i, &mesaj, sizeof(mesaj), 0);
								break;

							//trimitere informatii despre un client
							case 3:
								mesaj = get_cli_addr(clienti, mesaj.payload);
								send(i, &mesaj, sizeof(mesaj), 0);
								break;
												
							//adaugare fisier in lista
							case 4:
								add_file (i, clienti, mesaj.payload);
								break;

							//scoatere fisier din lista
							case 5:
								remove_file (i, clienti, mesaj.payload);
								break;
							
							//se intorc informatii cu fisierele partajate de un client
							case 6:
								
								mesaj = get_share(clienti, mesaj.payload);
								send(i, &mesaj, sizeof(mesaj), 0);
								break;

							//trimitere informatii despre un client pentru transfer de fisiere
							case 7:
								mesaj = get_cli_addr_f(clienti, mesaj.payload);
								send(i, &mesaj, sizeof(mesaj), 0);
								break;

							default:
								printf ("Nu exista acest tip de mesaj.\n");
						}
						
					}
				} 
			}
		}
	}

	close(sockfd);
	return 0; 
}


