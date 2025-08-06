#include <stdio.h>
#include <winsock.h>
#include <stdlib.h>

#define RCVBUFSIZE 50  /* Dimensiunea buffer-ului */
#define MAXPENDING 5   /* Nr. maxim de conexiuni/cereri */

void ExitCuEroare(char* errorMessage) {
    fprintf(stderr, "%s: %d\n", errorMessage, WSAGetLastError());
    exit(1);
}

void HandleTCPClient(int clntSocket) {
    char strBuffer[RCVBUFSIZE];
    int recvMsgSize;

    while (1) {
        if ((recvMsgSize = recv(clntSocket, strBuffer, RCVBUFSIZE - 1, 0)) <= 0)
            break;

        strBuffer[recvMsgSize] = '\0';

        // Verificam daca clientul a trimis mesajul de oprire
        if (strcmp(strBuffer, "STOP") == 0) {
            printf("Client a inchis conexiunea.\n");
            break;
        }

        int num = atoi(strBuffer);
        num++; // Incrementare

        sprintf(strBuffer, "%d", num);
        send(clntSocket, strBuffer, strlen(strBuffer), 0);
    }

    closesocket(clntSocket);
}

void main(int argc, char* argv[]) {
    int servSock, clntSock;
    struct sockaddr_in servAddr, clntAddr;
    unsigned short servPort;
    unsigned int clntLen;
    WSADATA wsaData;

    if (argc != 2) {
        fprintf(stderr, "Utilizare:  %s <Port Server>\n", argv[0]);
        exit(1);
    }

    servPort = atoi(argv[1]);

    if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup() a esuat!");
        exit(1);
    }

    if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        ExitCuEroare("socket() a esuat!");

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(servPort);

    if (bind(servSock, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0)
        ExitCuEroare("bind() a esuat!");

    if (listen(servSock, MAXPENDING) < 0)
        ExitCuEroare("listen() a esuat!");

    printf("Server asculta pe portul %d...\n", servPort);

    while (1) {
        clntLen = sizeof(clntAddr);
        if ((clntSock = accept(servSock, (struct sockaddr*)&clntAddr, &clntLen)) < 0)
            ExitCuEroare("accept() a esuat!");

        printf("Conectare client: %s\n", inet_ntoa(clntAddr.sin_addr));

        HandleTCPClient(clntSock);
    }
}