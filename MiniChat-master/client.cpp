#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Messages.h"

#define MAX_CLIENTS 20
#define BUFLEN 1024
using namespace std;

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
	int sockfd, newsockfd, portno, cont = 1, serverfd;
	unsigned int clilen;
	char nume[50], buffer[BUFLEN], buffer1[BUFLEN], buffer2[BUFLEN];
	struct sockaddr_in serv_addr, cli_addr, my_addr;
	int n, i, j;
	msg mesaj;
	vector<pair<char*, char*> > mesaje;
	vector<pair<char*, char*> > fisiere;
	vector<pair<FILE*, int> > transfer, receive;
	vector<char*> infoc;
	FILE* f;
	struct timeval timeout;

	fd_set read_fds;
	fd_set tmp_fds;
	int fdmax;

	if (argc < 5) {
		fprintf(stderr,"Usage : %s nume addr_server port port_listen\n", argv[0]);
		exit(1);
	}
	
	//se fac initializarile
	strcpy(nume, argv[1]);
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);
     
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
		error("Nu s-a putut deschide soketul");
     
	portno = atoi(argv[4]);
	timeout.tv_sec = 0;
	timeout.tv_usec = 10000;


	//se face bind pe portul propriu
	memset((char *) &my_addr, 0, sizeof(serv_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = INADDR_ANY;	// foloseste adresa IP a masinii
	my_addr.sin_port = htons(portno);
     
	if (bind(sockfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr)) < 0) 
		error("Eroare la bind");
     
	listen(sockfd, MAX_CLIENTS);

	//adaugam soketul pe care se asculta conexiuni si stdin in multimea read_fds
	FD_SET(0, &read_fds);
	FD_SET(sockfd, &read_fds);
	fdmax = sockfd;

	//conectare la server
	serverfd = socket(AF_INET, SOCK_STREAM, 0);
     	if (serverfd < 0) 
        	error("Nu s-a putut deschide soketul");
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	inet_aton(argv[2], &serv_addr.sin_addr);
	if (connect(serverfd,(struct sockaddr*) &serv_addr,sizeof(serv_addr)) < 0) 
		error("Eroare la conectare"); 

	//trimitere date de identificare si asteptare acceptare conexiune
	mesaj.type  = 0;
	sprintf(mesaj.payload, "%i %s %s", portno, nume);
	send(serverfd, &mesaj, sizeof(mesaj), 0);
	recv(serverfd, &mesaj, sizeof(mesaj), 0);
	if (mesaj.type != 0) {
		printf("Conexiune refuzata: %s.\n", mesaj.payload);
		return 0;
	}
	else
		printf("Conexiune acceptata.\n"); 

	//adauga serverul la lista porturilor pe care se asculta
	FD_SET(serverfd, &read_fds);
	if (serverfd > fdmax)
		fdmax = serverfd;

	// main loop
	while (cont) {
		tmp_fds = read_fds; 
		if (select(fdmax + 1, &tmp_fds, NULL, NULL, &timeout) == -1) 
			error("Eroare la select");

		//se transfera 1KB din fiecare fisier care este transferat la timpul curent
		for (i = 0; i < transfer.size(); i++) {
			mesaj.type = 10;
			if ((mesaj.info = fread(mesaj.payload, 1, sizeof(mesaj.payload), transfer[i].first)) > 0) {
				send(transfer[i].second, &mesaj, sizeof(mesaj), 0);
			}
			else {
				//daca nu mai sunt date se inchide fisierul si se trimite mesaj ca fisierul s-a transferat
				fclose(transfer[i].first);
				//se afiseaza mesajul de incheiere
				sscanf(infoc[i], "%s%s", buffer, buffer1);
				printf("S-a trimis fisierul %s clientului %s.\n", buffer, buffer1);
				free(infoc[i]);
				infoc.erase(infoc.begin() + i);
				mesaj.type = 11;
				send(transfer[i].second, &mesaj, sizeof(mesaj), 0);
				transfer.erase(transfer.begin() + i);
			}
		}
	
		for(i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				
				//s-a primit o comanda
				//se identifica si se trateaza corespunzator
				if (i == 0) {
					mesaj.type = -1;
					memset(mesaj.payload, 0, sizeof(mesaj));

					fgets(buffer1, BUFLEN, stdin);
					sscanf(buffer1, "%s", buffer);
					n = strlen(buffer) + 1;

					if (strcmp(buffer, "clientlist") == 0) {
						mesaj.type = 1;
						goto snd;
					}

					if (strcmp(buffer, "infoclient") == 0) {
						mesaj.type = 2;
						sscanf(buffer1 + n, "%s", mesaj.payload);
						goto snd;
					}

					if (strcmp(buffer, "message") == 0) {
						mesaj.type = 3;
						sscanf(buffer1 + n, "%s", mesaj.payload);
						strcpy(buffer, buffer1 + n + strlen(mesaj.payload) + 1);
						mesaje.push_back( pair<char*, char*>(strdup(mesaj.payload), strdup(buffer)));
						goto snd;
					}

					if (strcmp(buffer, "sharefile") == 0) {
						mesaj.type = 4;
						sscanf(buffer1 + n, "%s", mesaj.payload);
						goto snd;
					}

					if (strcmp(buffer, "unsharefile") == 0) {
						mesaj.type = 5;
						sscanf(buffer1 + n, "%s", mesaj.payload);
						goto snd;
					}

					if (strcmp(buffer, "getshare") == 0) {
						mesaj.type = 6;
						sscanf(buffer1 + n, "%s", mesaj.payload);
						goto snd;
					}

					if (strcmp(buffer, "getfile") == 0) {
						mesaj.type = 7;
						sscanf(buffer1 + n, "%s %s", mesaj.payload, buffer);
						fisiere.push_back(pair<char*, char*>(strdup(mesaj.payload), strdup(buffer)));
						strcat(mesaj.payload, " ");
						strcat(mesaj.payload, buffer);
						goto snd;
					}


					if (strcmp(buffer, "quit") == 0) {
						//se inchid toate conexiunile se opreste programul
						for (j = 1; j <= fdmax; j++)
							if (FD_ISSET(j, &read_fds))
								close(j);
						cont = 0;
						break;
					}
					else
						printf("Comanda gresita\n");


					snd:
					if (mesaj.type != -1)
						send(serverfd, &mesaj, sizeof(mesaj), 0);

					continue;
					
				}
			
				if (i == sockfd) {
					// a venit ceva pe socketul de ascultare = o noua conexiune
					clilen = sizeof(cli_addr);
					if ((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen)) == -1) {
						error("Eroare la accept");
					} 
					else {
						//adaug noul socket intors de accept() la multimea descriptorilor de citire
						FD_SET(newsockfd, &read_fds);
						if (newsockfd > fdmax) { 
							fdmax = newsockfd;
						}
					}
				}
					
				else {
					//s-au primit date de la clienti sau de la server

					if ((n = recv(i, &mesaj, sizeof(mesaj), 0)) <= 0) {
						if (n == 0) {
							//in cazuli in care conexiunea cu serverul s-a inchis se opreste clientul
							if (i == serverfd) {
								printf("S-a inchis serverul.\n");
								for (j = 1; j <= fdmax; j++)
									if (FD_ISSET(j, &read_fds))
										close(j);
								cont = 0;
								break;
							}
	
							//in caz ca a aparut o eroare de transfer a unui fisier se afiseaza un mesaj
							for (j = 0; j < receive.size(); j++)
								if (receive[j].second == i) {
									fclose(receive[j].first);
									receive.erase(receive.begin() + j);
									sscanf(infoc[j], "%s%s", buffer, buffer1);
									free(infoc[j]);
									printf("Eroare transfer fisier %s de la clientul %s.\n",
											 buffer, buffer1);
									infoc.erase(infoc.begin() + j);
									break;
								}	
						}
									
						 else 
							error("Eroare la recv");
			
						close(i); 
						FD_CLR(i, &read_fds); // scoatem din multimea de citire socketul care s-a inchis 
					} 
					
					else { //s-a primit un mesaj
						switch (mesaj.type) {

							//se afiseaza o lista cu toti clientii
							case 1:
								printf("Clienti conectati: %s\n", mesaj.payload);
								break;
							
							//se afiseaza informatii despre un client
							case 2:
								if (mesaj.info == 1)
									printf("Clientul %s nu exista.\n", mesaj.payload);
								else
									printf("%s\n", mesaj.payload);
								break;
						
							//se deschide o noua conexiune, si se trimite mesajul
							case 3:
								//se afiseaza eroarea, in caz ca exista
								if (mesaj.info == 1) {
									printf("Clientul %s nu exista.\n", mesaj.payload);
									break;
								}
								newsockfd = socket(AF_INET, SOCK_STREAM, 0);
     								if (newsockfd < 0) 
        								error("Nu s-a putut deschide soketul");
								
								//se extrag datele conexiunii din mesajul de la server
								//se creaza o noua conexiune
								sscanf(mesaj.payload, "%s%s%s", buffer, buffer1, buffer2);
								cli_addr.sin_family = AF_INET;
								cli_addr.sin_port = htons(atoi(buffer1));
								inet_aton("127.0.0.1", &cli_addr.sin_addr);
								if (connect(newsockfd,(struct sockaddr*) &cli_addr,sizeof(cli_addr)) < 0) {
									printf("Eroare la conectare\n");
									break;
								}
								//se cauta si se copiaza mesajul de trimis
								for (j = 0; j < mesaje.size(); j++)
									if (strcmp(mesaje[j].first, buffer) == 0) {
										mesaj.type = 8;
										sprintf(mesaj.payload, "%s %s", nume, mesaje[j].second);
										free(mesaje[j].first);
										free(mesaje[j].second);
										mesaje.erase(mesaje.begin() + j);	
										break; 
									}
								send(newsockfd, &mesaj, sizeof(mesaj), 0);
								FD_SET(newsockfd, &read_fds);
								if (newsockfd > fdmax) 
									fdmax = newsockfd;
								break;
												
							//se afiseaza informatii cu fisierele partajate de un client
							case 6:
								sscanf(mesaj.payload, "%s", buffer);
								if (mesaj.info == 1)
									printf("Clientul %s nu exista.\n", buffer);
								else
									printf("Fisierele partajate de clientul %s: %s\n", 
										buffer, mesaj.payload + strlen(buffer));
								break;

							//se deschide o noua conexiune, si se cere transferul unui fisier
							case 7:
								//afisare mesaje de eroare, in caz ca exista
								if (mesaj.info == 1) {
									printf("Clientul %s nu exista.\n", mesaj.payload);
									break;
								}
								else
									if (mesaj.info == 2) {
										sscanf(mesaj.payload, "%s%s", buffer, buffer1);
										printf("Clientul %s nu are fisierul %s.\n", buffer, buffer1);
										break;
									}

								newsockfd = socket(AF_INET, SOCK_STREAM, 0);
     								if (newsockfd < 0) 
        								error("Nu s-a putut deschide soketul");
								//se extrag datele conexiunii din mesajul de la server
								sscanf(mesaj.payload, "%s%s%i%s", buffer, buffer1, &n, buffer2);
								cli_addr.sin_family = AF_INET;
								cli_addr.sin_port = htons(n);
								inet_aton(buffer2, &cli_addr.sin_addr);
								if (connect(newsockfd,(struct sockaddr*) &cli_addr,sizeof(cli_addr)) < 0) {
									printf("Eroare la conectare\n");
									break;
								}
				
								//adauga noul socket la multimea descriptorilor
								FD_SET(newsockfd, &read_fds);
								if (newsockfd > fdmax) { 
									fdmax = newsockfd;
								}

								//se deschide fisierul pentru transfer si se pregateste transferul
								for (j = 0; j < fisiere.size(); j++)
									if (strcmp(fisiere[j].second, buffer) == 0) {
										mesaj.type = 9;
										sprintf(mesaj.payload, "%s %s", fisiere[j].second, nume);
										strcpy(buffer2, fisiere[j].second);
										strcpy(buffer, fisiere[j].second);
										strcat(buffer, "_primit");
										free(fisiere[j].first);
										free(fisiere[j].second);
										fisiere.erase(fisiere.begin() + j);
										f = fopen(buffer, "w");
										if (f == NULL) {
											printf("Nu se poate crea fisierul %s.\n", buffer);
											break;
										}
										receive.push_back(pair<FILE*, int> (f, newsockfd));
										printf("Se primeste fisierul %s de la clientului %s.\n", 
												buffer2, buffer1);
										strcat(buffer2," ");
										strcat(buffer2, buffer1);
										infoc.push_back(strdup(buffer2)); 
										
										break; 
									}
								if (f != NULL)
									send(newsockfd, &mesaj, sizeof(mesaj), 0);
								break;
							
							//se afiseaza mesajul nou primit si se inchide conexiunea
							case 8:
								sscanf(mesaj.payload, "%s", buffer);
								printf("S-a primit urmatorul mesaj de la %s: %s",
										buffer, mesaj.payload + strlen(buffer));
								close(i);
								FD_CLR(i, &read_fds);
								break;
							
							//se deschide fisierul cerut si se adauga in lista de transfer
							case 9:
								sscanf(mesaj.payload, "%s%s", buffer, buffer1);
								f = fopen(buffer, "r");
								if (f == NULL) {
									printf("Eroare deschidere fisier %s.\n", buffer);
									close(i);
									FD_CLR(i, &read_fds);
									break;
								}
								printf("Se trimite fisierul %s clientului %s\n", buffer, buffer1);
								transfer.push_back(pair<FILE*,int> (f, i));
								infoc.push_back(strdup(mesaj.payload)); 
								break;

							//se scrie in fisier in caz ca se primesc date din acesta
							case 10:
								for (j = 0; j < receive.size(); j++)
									if (receive[j].second == i) {
										fwrite (mesaj.payload, 1, mesaj.info, receive[j].first);
										break;
									}
								break;

							//transferul unui fisier s-a terminat
							//se afiseaza mesaj si se inchid conexiunea si fisierul
							case 11:
								for (j = 0; j < receive.size(); j++)
									if (receive[j].second == i) {
										fclose(receive[j].first);
										receive.erase(receive.begin() + j);
										sscanf(infoc[j], "%s%s", buffer, buffer1);
										free(infoc[j]);
										printf("S-a transferat fisierul %s de la clientul %s.\n",
												 buffer, buffer1);
										infoc.erase(infoc.begin() + j);
										close(i);
										FD_CLR(i, &read_fds);
										break;
									}
							break;
								

							default:
								break;
						}
			
					}

				} 
			}
		}
	}

	close(serverfd);
	close(sockfd);
   
     return 0; 
}
