//
//
//  Utillizarea acestui header impreuna cu service.c
//  simplifica procesul de scriere a unui serviciu.
//  Pentru implementarea unui nou serviciu, este nevoie numai de 
//  scrierea functiilor ServiceStart() si ServiceStop().
//  
//  Nu este nevoide de modificarea fisierului service.c.  
//  La proiect trebuie adaugate bibliotecile:
//
//  libcmt.lib kernel32.lib advapi.lib shell32.lib (ws2_32.lib)
//
//  Acest lucru se realizeaza modificand fisierul MAKEFILE.
//
//  Dupa compilare, sunt disponibile urmatoarele optiuni:
//
//  <service exe> -?                afiseaza lista de optiuni
//  <service exe> -install          instaleaza serviciul
//  <service exe> -remove           elimina serviciul
//  <service exe> -debug <params>   ruleaza ca si aplicatie consola pt depanare
//
//  Nota: Codul implementeaza handlere pentru Ctrl+C and Ctrl+Break 
//        in modul depanare.  Aceste evenimente cauzeaza apelul 
//        rutinei ServiceStop.
//
//        Pentru controlul serviciului ( start, stop, etc )
//        se poate utiliza Contol Panel-Administrative Tools-Services,
//        comanda NET sau programul SC.
//
//
#define _CRT_SECURE_NO_WARNINGS

#ifndef _SERVICE_H
#define _SERVICE_H


#ifdef __cplusplus
extern "C" {
#endif


//////////////////////////////////////////////////////////////////////////////
////
// numele executabilului
#define SZAPPNAME            "InfoService"
// numele intern al serviciului
#define SZSERVICENAME        "InfoService"
// numele afisat al serviciului
#define SZSERVICEDISPLAYNAME "Info Service - Laborator API "
// lista dependentelor serviciului 
#define SZDEPENDENCIES       ""
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
//// Startare serviciu

VOID ServiceStart(DWORD dwArgc, LPTSTR *lpszArgv);

////  Stopare serviciu
VOID ServiceStop();
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
//// Urmatoarele functii sunt apelate din ServiceStart, ServiceStop
//// dar nu necesita implementare specifica.
//// Ambele sunt implementate in service.c
//////////////////////////////////////////////////////////////////////////////
//
//  ReportStatusToSCMgr()
//
//  Seteaza starea curenta a serviciului si o raporteaza 
//  catre Service Control Manager
//
//  PARAMETRI:
//    dwCurrentState - starea serviciului
//    dwWin32ExitCode - codul de eroare
//    dwWaitHint - timpul scurs pana la urmatoarea verificare, in cel mai defavorabil caz 
//
//  RETURNEAZA:
//    TRUE  - succes
//    FALSE - eroare
//
BOOL ReportStatusToSCMgr(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);


//
//  AddToMessageLog(LPTSTR lpszMsg)
//
//  Permite oricarui fir sa jurnalizeze un mesaj de eroare 
//
//  PARAMETRI:
//    lpszMsg - textul mesajului
//
//  RETURNEAZA:
//    nimic
//
void AddToMessageLog(LPTSTR lpszMsg);
//////////////////////////////////////////////////////////////////////////////


#ifdef __cplusplus
}
#endif

#endif
