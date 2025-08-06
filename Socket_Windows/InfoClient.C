#include <stdio.h>
#include <winsock.h>
#include <stdlib.h>

#define RCVBUFSIZE 50  /* Dimensiunea buffer-ului */

void ExitCuEroare(char* errorMessage) {
    fprintf(stderr, "%s: %d\n", errorMessage, WSAGetLastError());
    exit(1);
}

void main(int argc, char* argv[]) {
    int sock;
    struct sockaddr_in servAddr;
    unsigned short servPort;
    char* servIP;
    char strBuffer[RCVBUFSIZE];
    int n, i;
    int bytesRcvd;
    WSADATA wsaData;

    if (argc != 3) {
        fprintf(stderr, "Utilizare: %s <Server IP> <Port>\n", argv[0]);
        exit(1);
    }

    servIP = argv[1];
    servPort = atoi(argv[2]);

    if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup() a esuat!");
        exit(1);
    }

    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        ExitCuEroare("socket() a esuat!");

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr(servIP);
    servAddr.sin_port = htons(servPort);

    if (connect(sock, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0)
        ExitCuEroare("connect() a esuat!");

    printf("Introduceti un numar n: ");
    scanf_s("%d", &n);

    for (i = 0; i < n; i++) {
        printf(strBuffer, "%d", i);
        if (send(sock, strBuffer, strlen(strBuffer), 0) != strlen(strBuffer))
            ExitCuEroare("send() a esuat!");

        if ((bytesRcvd = recv(sock, strBuffer, RCVBUFSIZE - 1, 0)) <= 0)
            ExitCuEroare("recv() a esuat!");

        strBuffer[bytesRcvd] = '\0';
        printf("Server a trimis: %s\n", strBuffer);
    }

    // Trimitem mesaj de inchidere
    strcpy_s(strBuffer, sizeof(strBuffer), "STOP");
    send(sock, strBuffer, strlen(strBuffer), 0);

    closesocket(sock);
    WSACleanup();
}
