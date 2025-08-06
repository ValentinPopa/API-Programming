//
//  service.c
//
//  Implementeaza functiile necesare serviciilor Windows
//
//
//  FUNCTII:
//    main(int argc, char **argv);
//    service_ctrl(DWORD dwCtrlCode);
//    service_main(DWORD dwArgc, LPTSTR *lpszArgv);
//    CmdInstallService();
//    CmdRemoveService();
//    CmdDebugService(int argc, char **argv);
//    ControlHandler ( DWORD dwCtrlType );
//    GetLastErrorText( LPTSTR lpszBuf, DWORD dwSize );
//
//
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <tchar.h>

#include "service.h"



// variabile interne
SERVICE_STATUS          ssStatus; // starea curenta a serviciului
SERVICE_STATUS_HANDLE   sshStatusHandle;
DWORD                   dwErr = 0;
BOOL                    bDebug = FALSE;
TCHAR                   szErr[256];

// prototipuri de functii
VOID WINAPI service_ctrl(DWORD dwCtrlCode);
VOID WINAPI service_main(DWORD dwArgc, LPTSTR *lpszArgv);
VOID CmdInstallService();
VOID CmdRemoveService();
VOID CmdDebugService(int argc, char **argv);
BOOL WINAPI ControlHandler ( DWORD dwCtrlType );
LPTSTR GetLastErrorText( LPTSTR lpszBuf, DWORD dwSize );

//
//  main()
//
//  Punctul de intrare al serviciului
//
//  PARAMETRII:
//    argc - numarul de argumente in linia de comanda
//    argv - vectorul argumentelor din linia de comanda
//
//  FUNCTIONALITATE:
//    main() executa actiunile aferente parametrilor din linia de comanda,
//    atunci cand este invocata de utilizator.
//    Service Control Manager invoca main() atunci cand starteaza serviciul.
//    In acest caz, din main() se apeleaza StartServiceCtrlDispatcher pentru 
//    a inregistra firul principal al procesului care gazduieste serviciul. 
//    StartServiceCtrlDispatcher conecteaza firul principal cu 
//    Service Control Manager, si nu returneaza decat
//    atunci cand serviciul se termina.
//
void main(int argc, char **argv)
{
    SERVICE_TABLE_ENTRY dispatchTable[] =
    {
        { TEXT(SZSERVICENAME), (LPSERVICE_MAIN_FUNCTION)service_main },
        { NULL, NULL }
    };

    if ( (argc > 1) &&
         ((*argv[1] == '-') || (*argv[1] == '/')) )
    {
        if ( _stricmp( "install", argv[1]+1 ) == 0 )
        {
            CmdInstallService();
        }
        else if ( _stricmp( "remove", argv[1]+1 ) == 0 )
        {
            CmdRemoveService();
        }
        else if ( _stricmp( "debug", argv[1]+1 ) == 0 )
        {
            bDebug = TRUE;
            CmdDebugService(argc, argv);
        }
        else
        {
            goto dispatch;
        }
        exit(0);
    }

    // 
    // Service Control Manager, la startarea serviciului
    // va astepta ca procesul sa apeleze StartServiceCtrlDispatcher.
	// Se va stabili o conexiune intre firul de executie principal al
	// procesului si Service Control Manager. Prin intermediul acestei
	// conexiuni, Service Control Manager va trimite cererea de startare 
	// a serviciului catre firul principal, care actioneaza ca un dispecer.
	// El va crea un nou fir de executie pentru a executa functia asociata
	// serviciului prin tabela SERVICE_TABLE_ENTRY.
	// In cazul concret al acestei aplicatii, procesul gazduieste un 
	// singur serviciu, identificat in cadrul tabelei prin numele
	// "InfoService" a carui functie asociata este "service_main".
	//
    dispatch:
        
        printf( "%s -install          pentru instalare serviciu\n", SZAPPNAME );
        printf( "%s -remove           pentru dezinstalare serviciu\n", SZAPPNAME );
        printf( "%s -debug <params>   rulare ca si consola pentru depanare\n", SZAPPNAME );
        printf( "\nVa fi apelata StartServiceCtrlDispatcher.\n" );
        printf( "Stabilirea conexiunii poate dura cateva secunde. Asteptati...\n" );

        if (!StartServiceCtrlDispatcher(dispatchTable))
            AddToMessageLog(TEXT("StartServiceCtrlDispatcher failed."));
}

//
//  service_main()
//
//  Executa initializarea serviciului
//
//  PARAMETRII:
//    argc - numarul de argumente in linia de comanda
//    argv - vectorul argumentelor din linia de comanda
//
//
//  FUNCTIONALITATE:
//    Realizeaza initializarea serviciului si apoi invoca 
//    rutina ServiceStart() definita de utilizator.
//    Practic intreaga functionalitate a serviciului
//    poate fi implementata in functia utilizator ServiceStart().
//
void WINAPI service_main(DWORD dwArgc, LPTSTR *lpszArgv)
{

    // Se inregistreaza o rutina Service Control Handler
    // (service_ctrl)
	//
    sshStatusHandle = RegisterServiceCtrlHandler( TEXT(SZSERVICENAME), service_ctrl);

    if (!sshStatusHandle)
        goto cleanup;

    // campuri ale structurii SERVICE_STATUS 
    // care raman neschimbate
	//
    ssStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ssStatus.dwServiceSpecificExitCode = 0;


    // Raporteaza starea catre Service Control Manager.
    //
    if (!ReportStatusToSCMgr(
        SERVICE_START_PENDING, // stare serviciu
        NO_ERROR,              // cod de iesire
        3000))                 // asteptare
        goto cleanup;

    // rutina definita de utilizator
    ServiceStart( dwArgc, lpszArgv );

cleanup:

    // incercare de a raporta starea de stop catre Service Control Manager.
    //
    if (sshStatusHandle)
        (VOID)ReportStatusToSCMgr(
                            SERVICE_STOPPED,
                            dwErr,
                            0);

    return;
}



//
//  service_ctrl()
//
//  Aceasta functie este apelata de catre Service Control Manager.
//  ori de cate ori este invocata  ControlService() pentru acest serviciu
//
//  PARAMETRII:
//    dwCtrlCode - tipul de control solicitat
//
//
VOID WINAPI service_ctrl(DWORD dwCtrlCode)
{
    // Trateaza codul de control solicitat
    //
    switch(dwCtrlCode)
    {
        // Stopare serviciu
        //
        // Starea SERVICE_STOP_PENDING trebuie raportata inainte
        // de a semnala evenimentul de stop - hServerStopEvent - in
        // ServiceStop().  Altfel poate aparea eroarea 1053:
        // "The Service did not respond..."
        // 
        case SERVICE_CONTROL_STOP:
            ReportStatusToSCMgr(SERVICE_STOP_PENDING, NO_ERROR, 0);
            ServiceStop();
            return;

        // actualizare stare serviciu
        //
        case SERVICE_CONTROL_INTERROGATE:
            break;

        // cod de control invalid
        //
        default:
            break;

    }

    ReportStatusToSCMgr(ssStatus.dwCurrentState, NO_ERROR, 0);
}


//
//  ReportStatusToSCMgr()
//
//  Seteaza starea curenta a serviciului si o 
//  raporteaza catre Service Control Manager
//
//  PARAMETRII:
//    dwCurrentState - starea serviciului
//    dwWin32ExitCode - codul de eroare de raportat
//    dwWaitHint - durata max de executie pana la urmatorul 
//		           apel SetServiceStatus(), al carui parametru
//                 de tip SERVICE_STATUS este modificat astfel:
//                    - dwCheckPoint este incrementat sau
//                    - dwCurrentState s-a schimbat
//                 (un nou CheckPoint sau s-a schimbat starea
//                  serviciului)
//  RETURNEAZA:
//    TRUE  - succes
//    FALSE - eroare
//
//
BOOL ReportStatusToSCMgr(DWORD dwCurrentState,
                         DWORD dwWin32ExitCode,
                         DWORD dwWaitHint)
{
    static DWORD dwCheckPoint = 1;
    BOOL fResult = TRUE;


    if ( !bDebug ) // la depanare, nu raportam catre Service Control Manager
    {
        if (dwCurrentState == SERVICE_START_PENDING)
            ssStatus.dwControlsAccepted = 0;
        else
            ssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

        ssStatus.dwCurrentState = dwCurrentState;
        ssStatus.dwWin32ExitCode = dwWin32ExitCode;
        ssStatus.dwWaitHint = dwWaitHint;

        if ( ( dwCurrentState == SERVICE_RUNNING ) ||
             ( dwCurrentState == SERVICE_STOPPED ) )
            ssStatus.dwCheckPoint = 0;
        else
            ssStatus.dwCheckPoint = dwCheckPoint++;


        // Raporteaza starea serviciului catre Service Control Manager
        //
        if (!(fResult = SetServiceStatus( sshStatusHandle, &ssStatus))) {
            AddToMessageLog(TEXT("SetServiceStatus"));
        }
    }
    return fResult;
}


//
//  AddToMessageLog()
//
//  Permite logarea mesajelor de eroare
//
//  PARAMETRII:
//    lpszMsg - textul mesajului
//
//
VOID AddToMessageLog(LPTSTR lpszMsg)
{
    TCHAR   szMsg[256];
    HANDLE  hEventSource;
    LPTSTR  lpszStrings[2];


    if ( !bDebug )
    {
        dwErr = GetLastError();

        // Utilizeaza "event logging" pentru jurnalizare erori
        //
        hEventSource = RegisterEventSource(NULL, TEXT(SZSERVICENAME));

        _stprintf(szMsg, TEXT("%s eroare: %d"), TEXT(SZSERVICENAME), dwErr);
        lpszStrings[0] = szMsg;
        lpszStrings[1] = lpszMsg;

        if (hEventSource != NULL) {
            ReportEvent(hEventSource, // handle la sursa evenimentului
                EVENTLOG_ERROR_TYPE,  // tip eveniment
                0,                    // categorie eveniment
                0,                    // ID eveniment
                NULL,                 // SID curent utilizator
                2,                    // nr de mesaje in lpszStrings
                0,                    // nr octeti date binare
                lpszStrings,          // vectorul de pointeri la sirurile cu mesaje
                NULL);                // nu exista date binare

            (VOID) DeregisterEventSource(hEventSource);
        }
    }
}


///////////////////////////////////////////////////////////////////
//
// Codul urmator este responsabil de instalarea/dezinstalarea 
// serviciului 
// 
//
//
//  CmdInstallService()
//
//  Instaleaza serviciul
//
void CmdInstallService()
{
    SC_HANDLE   schService;
    SC_HANDLE   schSCManager;

    TCHAR szPath[512];

	// determina calea completa spre fisierul executabil al procesului
    if ( GetModuleFileName( NULL, szPath, 512 ) == 0 )
    {
        _tprintf(TEXT("Nu pot instala %s - %s\n"), TEXT(SZSERVICEDISPLAYNAME), 
			     GetLastErrorText(szErr, 256));
        return;
    }

    schSCManager = OpenSCManager(
                        NULL,                   // masina (NULL == locala)
                        NULL,                   // baza de date (NULL == implicita)
                        SC_MANAGER_ALL_ACCESS   // acces solicitat
                        );
    if ( schSCManager )
    {
		// SCM = Service Control Manager
        schService = CreateService(
            schSCManager,               // handle la baza de date SCM
            TEXT(SZSERVICENAME),        // numele intern al serviciului
            TEXT(SZSERVICEDISPLAYNAME), // numele afisat al serviciului
            SERVICE_ALL_ACCESS,         // acces dorit
            SERVICE_WIN32_OWN_PROCESS,  // tip serviciu
            SERVICE_DEMAND_START,       // modul de start 
            SERVICE_ERROR_NORMAL,       // modul de control al erorilor
            szPath,                     // calea spre executabil
            NULL,                       // serviciul nu apartine vreunui grup
            NULL,                       // fara identificator in cadrul grupului
            TEXT(SZDEPENDENCIES),       // dependente
            NULL,                       // cont LocalSystem 
            NULL);                      // fara parola

        if ( schService )
        {
            _tprintf(TEXT("%s instalat.\n"), TEXT(SZSERVICEDISPLAYNAME) );
            CloseServiceHandle(schService);
        }
        else
        {
            _tprintf(TEXT("CreateService a esuat - %s\n"), GetLastErrorText(szErr, 256));
        }

        CloseServiceHandle(schSCManager);
    }
    else
        _tprintf(TEXT("OpenSCManager a esuat - %s\n"), GetLastErrorText(szErr,256));
}



//
//  CmdRemoveService()
//
//  Stopeaza si dezinstaleaza serviciul
//
//
void CmdRemoveService()
{
    SC_HANDLE   schService;
    SC_HANDLE   schSCManager;

    schSCManager = OpenSCManager(
                        NULL,                   // masina (NULL == locala)
                        NULL,                   // baza de date (NULL == implicita)
                        SC_MANAGER_ALL_ACCESS   // acces solicitat
                        );
    if ( schSCManager )
    {
        schService = OpenService(schSCManager, TEXT(SZSERVICENAME), SERVICE_ALL_ACCESS);

        if (schService)
        {
            // incearca stoparea serviciului
            if ( ControlService( schService, SERVICE_CONTROL_STOP, &ssStatus ) )
            {
                _tprintf(TEXT("Stopare %s."), TEXT(SZSERVICEDISPLAYNAME));
                Sleep( 1000 );

                while( QueryServiceStatus( schService, &ssStatus ) )
                {
                    if ( ssStatus.dwCurrentState == SERVICE_STOP_PENDING )
                    {
                        _tprintf(TEXT("."));
                        Sleep( 1000 );
                    }
                    else
                        break;
                }

                if ( ssStatus.dwCurrentState == SERVICE_STOPPED )
                    _tprintf(TEXT("\n%s stopat.\n"), TEXT(SZSERVICEDISPLAYNAME) );
                else
                    _tprintf(TEXT("\n%s nu a putut fi stopat.\n"), TEXT(SZSERVICEDISPLAYNAME) );

            }

            // dezinstalare serviciu
            if( DeleteService(schService) )
                _tprintf(TEXT("%s dezinstalat.\n"), TEXT(SZSERVICEDISPLAYNAME) );
            else
                _tprintf(TEXT("DeleteService a esuat - %s\n"), GetLastErrorText(szErr,256));


            CloseServiceHandle(schService);
        }
        else
            _tprintf(TEXT("OpenService a esuat - %s\n"), GetLastErrorText(szErr,256));

        CloseServiceHandle(schSCManager);
    }
    else
        _tprintf(TEXT("OpenSCManager a esuat - %s\n"), GetLastErrorText(szErr,256));
}




///////////////////////////////////////////////////////////////////
//
//  Codul urmator este responsabil pentru rularea serviciului
//  ca si aplicatie consola
//
//
//  CmdDebugService(int argc, char ** argv)
//
//  Ruleaza serviciul ca si aplicatie consola
//
//  PARAMETRII:
//    argc - numarul de argumente in linia de comanda
//    argv - vectorul argumentelor din linia de comanda
//
//

void CmdDebugService(int argc, char ** argv)
{
    DWORD dwArgc;
    LPTSTR *lpszArgv;

#ifdef UNICODE
    lpszArgv = CommandLineToArgvW(GetCommandLineW(), &(dwArgc) );
#else
    dwArgc   = (DWORD) argc;
    lpszArgv = argv;
#endif

    _tprintf(TEXT("Depanare %s.\n"), TEXT(SZSERVICEDISPLAYNAME));

    SetConsoleCtrlHandler( ControlHandler, TRUE );

    ServiceStart( dwArgc, lpszArgv );
}


//
//  ControlHandler ( DWORD dwCtrlType )
//
//  Trateaza evenimetele consola (Ctrl/C, Ctrl+Break )
//
//  PARAMETRII:
//    dwCtrlType - tipul evenimentului
//
//  RETURNEAZA:
//    True - tratat
//    False - netratat
//
//
BOOL WINAPI ControlHandler ( DWORD dwCtrlType )
{
    switch( dwCtrlType )
    {
        case CTRL_BREAK_EVENT:  // utilizeaza Ctrl+C or Ctrl+Break pentru a simula
        case CTRL_C_EVENT:      // SERVICE_CONTROL_STOP in mod depanare
            _tprintf(TEXT("Stopare %s.\n"), TEXT(SZSERVICEDISPLAYNAME));
            ServiceStop();
            return TRUE;
            break;

    }
    return FALSE;
}

//
//  GetLastErrorText
//
//  Formateaza mesajul de eroare al carui cod este 
//  returnat de GetLastError()
//
//  PARAMETRII:
//    lpszBuf - adresa buffer destinatie
//    dwSize - dimensiune buffer
//
//  RETURNEAZA:
//    adresa buffer destinatie
//
//
LPTSTR GetLastErrorText( LPTSTR lpszBuf, DWORD dwSize )
{
    DWORD dwRet;
    LPTSTR lpszTemp = NULL;

    dwRet = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_ARGUMENT_ARRAY,
                           NULL,
                           GetLastError(),
                           LANG_NEUTRAL,
                           (LPTSTR)&lpszTemp,
                           0,
                           NULL );

    // buffer-ul furnizat nu este suficient de mare
    if ( !dwRet || ( (long)dwSize < (long)dwRet+14 ) )
        lpszBuf[0] = TEXT('\0');
    else
    {
        lpszTemp[lstrlen(lpszTemp)-2] = TEXT('\0');  //elimina caracterele cr si newline
        _stprintf( lpszBuf, TEXT("%s (0x%x)"), lpszTemp, GetLastError() );
    }

    if ( lpszTemp )
        LocalFree((HLOCAL) lpszTemp );

    return lpszBuf;
}
