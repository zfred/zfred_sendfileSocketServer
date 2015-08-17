#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>

#include <string.h>
#include <unistd.h>

#include <sys/reboot.h>
#include <netdb.h>  /* To get defns of NI_MAXSERV and NI_MAXHOST */

#include "socket_messages.h"
#include <cstdint> // pour int32_t

using namespace std;


int DEFAULT_PORT = 12345;
char local_host_name[] = "lo"; // utilisé pour ne pas choisir cette interface pour le serveur.
char DEFAULT_IF[20];

void printHelp()
{
    cout << "-p port pour choisir un port, le port par défaut est : " << DEFAULT_PORT << endl;
}

int main(int argc, char *argv[])
{
    /*
     * Parse command line using getopt
     */
    int opt;
    bool verbose = false;

    while ((opt = getopt(argc, argv, "r?vp:")) != -1) {
        switch (opt) {
        case 'r':
            reboot(RB_POWER_OFF); // test du power off, eteind le PC mais l'utilisateur doit etre ROOT
            break;
        case 'v':
            verbose = true;
            break;
        case '?':
            printHelp();
            exit(EXIT_SUCCESS);
            break;
        case 'p':
            DEFAULT_PORT = atoi(optarg);
            break;
        default: /* '?' */
            fprintf(stderr, "Usage: %s [-p port] \n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    /*
     * Decouvrir l'adresse IP Local
     */
    struct ifaddrs *ifaddlist, *ifadd;
    int res = getifaddrs(&ifaddlist);
    if(res == -1){
        perror("pb getifaddrs");
        exit(EXIT_FAILURE);
    }

    for(ifadd = ifaddlist; ifadd != NULL; ifadd = ifadd->ifa_next ){
        if(ifadd == NULL)
            continue;

        char host[NI_MAXHOST];
        int family = ifadd->ifa_addr->sa_family;

        /* Display interface name and family (including symbolic
           form of the latter for the common families)
        */

        printf("%-8s %s (%d)\n", ifadd->ifa_name,
               (family == AF_PACKET) ? "AF_PACKET" :
                         (family == AF_INET) ? "AF_INET" :
                                   (family == AF_INET6) ? "AF_INET6" : "???", family);

        /* For an AF_INET* interface address, display the address */

        if (family == AF_INET || family == AF_INET6) {
            int s = getnameinfo(ifadd->ifa_addr,
                                (family == AF_INET) ? sizeof(struct sockaddr_in) :
                                                      sizeof(struct sockaddr_in6),
                                host, NI_MAXHOST,
                                NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                exit(EXIT_FAILURE);
            }

            printf("\t\taddress: <%s>\n", host);
            if( family == AF_INET)
            {
                if( strcmp(ifadd->ifa_name, local_host_name) != 0)
                {
                    strcpy(DEFAULT_IF, host);
                }
            }
        }
    }
    freeifaddrs(ifaddlist);

    // TODO decommenter la ligne en dessous
    //if (verbose)
    cout << DEFAULT_IF << endl;

    /*
     * Creation Serveur
     */
    if(verbose){
        cout << "port : " << DEFAULT_PORT << endl;
    }
    struct sockaddr_in serv_addr;
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(DEFAULT_IF);
    serv_addr.sin_port=htons(DEFAULT_PORT);

    // init socket
    int socketFD = socket(AF_INET, SOCK_STREAM, 0);

    // bind
    bind(socketFD, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    // wait for client
    listen(socketFD, 5);
    int connFD;
    connFD = accept(socketFD, (struct sockaddr*)NULL, NULL);

    bool socketIsOpen = true;
    while(socketIsOpen){
        // get file name
        char fileName[6400];
        int fileDesc = -1;
        int l = read(connFD, fileName, sizeof(fileName)-1);
        fileName[l] = '\0';
        cout << "read : " << l << "  " << fileName << endl;
        if (strcmp(fileName, "CLOSE_SOCKET") == 0 )
        {
            cout << "fin de com" << endl;
            socketIsOpen = false;
            break;
        }
        else{
            // create file
            fileDesc =  open(fileName, O_WRONLY | O_CREAT , S_IRWXU );
            if(fileDesc == -1) {
                cout << "file do not exist" << endl;
                exit(EXIT_FAILURE);
            }
        }
        // get data and write data in file
        while (socketIsOpen) {
            l = read(connFD, fileName, sizeof(fileName)-1);
            if(l == 0){
                cout << "ok fini"<< endl;
                break;
            }
            if(l == -1){
                cout << "erreur de lecture" << endl;
                break;
            }
            ssize_t ln = write(fileDesc, fileName, l);
            if(verbose)
                cout <<"reçu depuis la socket: " << l << "\nEcrit dans le fichier: " << ln << endl;
        } // transfert en cours

        fileDesc != -1 ? close(fileDesc): 0;

    } // socket is open
    close(connFD);
    close(socketFD);


    return EXIT_SUCCESS;
}

