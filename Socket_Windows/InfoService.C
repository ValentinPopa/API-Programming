#define _CRT_SECURE_NO_WARNINGS
#undef UNICODE

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <tchar.h>
#include "service.h"
#include <winsock.h>
#include <Tlhelp32.h>

#define RCVBUFSIZE 50  /* Dimensiunea buffer-ului */
#define MAXPENDING 5   /* Nr. maxim de conexiuni/cereri */


// acest eveniment este semnalat atunci cand
// serviciul trebuie sa se termine
HANDLE  hServerStopEvent = NULL;

// Functia pentru gestionarea erorilor
void ExitCuEroare(char* errorMessage) {
    fprintf(stderr, "%s: %d\n", errorMessage, WSAGetLastError());
    exit(1);
}

// Functia care manipuleaza clientul TCP
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

// Functia care porneste serviciul si serverul TCP
VOID ServiceStart(DWORD dwArgc, LPTSTR* lpszArgv) {
    WSADATA wsaData;
    int servSock, clntSock;
    struct sockaddr_in servAddr, clntAddr;
    unsigned short servPort;
    unsigned int clntLen;
    HANDLE hEvents[2] = { NULL, NULL };
    OVERLAPPED os;
    PSECURITY_DESCRIPTOR pSD = NULL;
    SECURITY_ATTRIBUTES sa;

    servPort = 8080; // Setăm portul la 8080
    // Initializare Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        ExitCuEroare("WSAStartup() a esuat!");
    }

    // Creaza un obiect eveniment pentru a opri serviciul
    hServerStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (hServerStopEvent == NULL) {
        ExitCuEroare("Crearea evenimentului esuata!");
    }

    hEvents[0] = hServerStopEvent;

    // Creaza socketul pentru serverul TCP
    if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        ExitCuEroare("socket() a esuat!");
    }

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(servPort);

    if (bind(servSock, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0)
        ExitCuEroare("bind() a esuat!");

    if (listen(servSock, MAXPENDING) < 0)
        ExitCuEroare("listen() a esuat!");

    printf("Server asculta pe 127.0.0.1:%d...\n", servPort);

    // Raporteaza starea catre Service Control Manager.
    if (!ReportStatusToSCMgr(SERVICE_RUNNING, NO_ERROR, 0)) {
        ExitCuEroare("Raportare status esuata!");
    }

    // Serviciul ruleaza, asteptand conexiuni
    while (1) {
        clntLen = sizeof(clntAddr);
        if ((clntSock = accept(servSock, (struct sockaddr*)&clntAddr, &clntLen)) < 0) {
            ExitCuEroare("accept() a esuat!");
        }

        printf("Conectare client: %s\n", inet_ntoa(clntAddr.sin_addr));
        HandleTCPClient(clntSock);

        // Verifica daca serviciul trebuie oprit
        if (WaitForMultipleObjects(2, hEvents, FALSE, 0) == WAIT_OBJECT_0) {
            printf("Serviciul s-a oprit.\n");
            break;
        }
    }

    // Curatire
    closesocket(servSock);
    WSACleanup();
}

void WriteLog(const char* msg) {
    FILE* logFile = fopen("server_log.txt", "a");
    if (logFile != NULL) {
        fprintf(logFile, "%s\n", msg);
        fclose(logFile);
    }
}
int main(int argc, char* argv[]) {
    ServiceStart(argc, argv); // Porneste serviciul si serverul TCP
    return 0;
}
