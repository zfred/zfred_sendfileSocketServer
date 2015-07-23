#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string.h>
#include <unistd.h>

using namespace std;

// decouvrir l'IP serveur... broadcast ou multicast
#define HELLO_GROUP "127.0.0.1"
#define HELLO_PORT 12345

int main()
{

    struct sockaddr_in serv_addr;
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(HELLO_GROUP);
    serv_addr.sin_port=htons(HELLO_PORT);


    // init socket
    int socketFD = socket(AF_INET, SOCK_STREAM, 0);

    // bind
    bind(socketFD, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    // wait for client
    listen(socketFD, 5);
    int connFD;
    connFD = accept(socketFD, (struct sockaddr*)NULL, NULL);

    // get file name
    char fileName[6400];
    int l = read(connFD, fileName, sizeof(fileName)-1);
    fileName[l] = '\0';
    cout << "read : " << l << "  " << fileName << endl;

    // create file
    int fileDesc = open(fileName, O_WRONLY | O_CREAT , S_IRWXU );
    if(fileDesc == -1) {
        cout << "file do not exist" << endl;
        exit(EXIT_FAILURE);
    }

    // get data
    while (true) {
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
        cout << l << '\t' << ln << endl;
    }
    close(connFD);
    close(socketFD);
    close(fileDesc);
    // write data in file


    cout << "Hello World!" << endl;
    return 0;
}

