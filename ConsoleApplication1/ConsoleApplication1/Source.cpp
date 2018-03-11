#include <winsock2.h>
#include <iptypes.h>
#include <iphlpapi.h>
#include <windows.h>
#include <iostream>
#include <stdio.h>
#include <winnetwk.h>

#ifndef UNICODE
#define UNICODE
#endif
#pragma comment(lib, "mpr.lib")
#pragma comment(lib, "netapi32.lib")

void DisplayStruct(int i, LPNETRESOURCE lpnrLocal);

typedef struct _ASTAT_
{
    ADAPTER_STATUS adapt;
    NAME_BUFFER NameBuff[30];
}ASTAT, *PASTAT;
ASTAT Adapter;
// Функция получения MAC адреса.
// На вход получает указатель на буфер, куда записывается строковое
// представление полученного MAC адреса.

void GetMacAddress()
{
    NCB Ncb;
    UCHAR uRetCode;
    LANA_ENUM lenum;
    int i;

    memset(&Ncb, 0, sizeof(Ncb));
    Ncb.ncb_command = NCBENUM;
    Ncb.ncb_buffer = (UCHAR *)&lenum;
    Ncb.ncb_length = sizeof(lenum);
    uRetCode = Netbios(&Ncb);

    for (i = 0; i < lenum.length; i++)
    {
        memset(&Ncb, 0, sizeof(Ncb));
        Ncb.ncb_command = NCBRESET;
        Ncb.ncb_lana_num = lenum.lana[i];

        uRetCode = Netbios(&Ncb);

        memset(&Ncb, 0, sizeof(Ncb));
        Ncb.ncb_command = NCBASTAT;
        Ncb.ncb_lana_num = lenum.lana[i];

        strcpy((char*)Ncb.ncb_callname, "*               ");
        Ncb.ncb_buffer = (unsigned char *)&Adapter;
        Ncb.ncb_length = sizeof(Adapter);

        uRetCode = Netbios(&Ncb);
        if (uRetCode == 0)
        {
            printf("The Ethernet Number on LANA %d is: % 02x % 02x % 02x % 02x % 02x % 02x\n",
                lenum.lana[i],
                Adapter.adapt.adapter_address[0],
                Adapter.adapt.adapter_address[1],
                Adapter.adapt.adapter_address[2],
                Adapter.adapt.adapter_address[3],
                Adapter.adapt.adapter_address[4],
                Adapter.adapt.adapter_address[5]);
        }
    }

}



BOOL WINAPI EnumerateFunc(LPNETRESOURCE lpnr)
{
    DWORD dwResult, dwResultEnum;
    HANDLE hEnum;
    DWORD cbBuffer = 16384;
    DWORD cEntries = -1; // Искать все объекты
    LPNETRESOURCE lpnrLocal;
    DWORD i;
    // Вызов функции WNetOpenEnum для начала перечисления компьютеров.
    dwResult = WNetOpenEnum(RESOURCE_GLOBALNET, // все сетевые ресурсы
        RESOURCETYPE_ANY, // все типы ресурсов
        0, // перечислить все ресурсы
        lpnr, // равно NULL при первом вызове функции
        &hEnum); // дескриптор ресурса
    if (dwResult != NO_ERROR)
    {
        // Обработка ошибок.
        printf("WnetOpenEnum failed with error %d\n", dwResult);
        //NetErrorHandler(hwnd, dwResult, (LPSTR)"WNetOpenEnum");
        return FALSE;
    }
    // Вызвов функции GlobalAlloc для выделения ресурсов.
    lpnrLocal = (LPNETRESOURCE)GlobalAlloc(GPTR, cbBuffer);
    if (lpnrLocal == NULL) {
        printf("WnetOpenEnum failed with error %d\n", dwResult);
        //      NetErrorHandler(hwnd, dwResult, (LPSTR)"WNetOpenEnum");
        return FALSE;
    }

    do
    {
        // Инициализируем буфер.
        ZeroMemory(lpnrLocal, cbBuffer);
        // Вызов функции WNetEnumResource для продолжения перечисления
        // доступных ресурсов сети.
        dwResultEnum = WNetEnumResource(hEnum,
            &cEntries, // Определено выше как -1
            lpnrLocal,
            &cbBuffer); // размер буфера
                        // Если вызов был успешен, то структуры обрабатываются циклом.
        if (dwResultEnum == NO_ERROR)
        {
            for (i = 0; i < cEntries; i++)
            {
                // Вызов определенной в приложении функции для отображения
                // содержимого структур NETRESOURCE.
                DisplayStruct(i, &lpnrLocal[i]);
                // Если структура NETRESOURCE является контейнером, то
                // функция EnumerateFunc вызывается рекурсивно.
                if (RESOURCEUSAGE_CONTAINER == (lpnrLocal[i].dwUsage
                    & RESOURCEUSAGE_CONTAINER))
                    if (!EnumerateFunc(&lpnrLocal[i]))
                        printf("EnumerateFunc returned FALSE\n");
                        //TextOut(hdc, 10, 10, "EnumerateFunc returned FALSE.", 29);
            }
        }
        // Обработка ошибок.
        else if (dwResultEnum != ERROR_NO_MORE_ITEMS)
        {
            printf("WNetEnumResource failed with error %d\n", dwResultEnum);
            //NetErrorHandler(hwnd, dwResultEnum, (LPSTR)"WNetEnumResource");
            break;
        }  
    }
    while (dwResultEnum != ERROR_NO_MORE_ITEMS);
        // Вызов функции GlobalFree для очистки ресурсов.
        GlobalFree((HGLOBAL)lpnrLocal);
        // Вызов WNetCloseEnum для остановки перечисления.
        dwResult = WNetCloseEnum(hEnum);
        if (dwResult != NO_ERROR)
        {
            // Обработка ошибок.
            printf("WNetCloseEnum failed with error %d\n", dwResult);
            //NetErrorHandler(hwnd, dwResult, (LPSTR)"WNetCloseEnum");
            return FALSE;
        }
        return TRUE;
}

void DisplayStruct(int i, LPNETRESOURCE lpnrLocal)
{
    printf("NETRESOURCE[%d] Scope: ", i);
    switch (lpnrLocal->dwScope) {
    case (RESOURCE_CONNECTED):
        printf("connected\n");
        break;
    case (RESOURCE_GLOBALNET):
        printf("all resources\n");
        break;
    case (RESOURCE_REMEMBERED):
        printf("remembered\n");
        break;
    default:
        printf("unknown scope %d\n", lpnrLocal->dwScope);
        break;
    }

    printf("NETRESOURCE[%d] Type: ", i);
    switch (lpnrLocal->dwType) {
    case (RESOURCETYPE_ANY):
        printf("any\n");
        break;
    case (RESOURCETYPE_DISK):
        printf("disk\n");
        break;
    case (RESOURCETYPE_PRINT):
        printf("print\n");
        break;
    default:
        printf("unknown type %d\n", lpnrLocal->dwType);
        break;
    }

    printf("NETRESOURCE[%d] DisplayType: ", i);
    switch (lpnrLocal->dwDisplayType) {
    case (RESOURCEDISPLAYTYPE_GENERIC):
        printf("generic\n");
        break;
    case (RESOURCEDISPLAYTYPE_DOMAIN):
        printf("domain\n");
        break;
    case (RESOURCEDISPLAYTYPE_SERVER):
        printf("server\n");
        break;
    case (RESOURCEDISPLAYTYPE_SHARE):
        printf("share\n");
        break;
    case (RESOURCEDISPLAYTYPE_FILE):
        printf("file\n");
        break;
    case (RESOURCEDISPLAYTYPE_GROUP):
        printf("group\n");
        break;
    case (RESOURCEDISPLAYTYPE_NETWORK):
        printf("network\n");
        break;
    default:
        printf("unknown display type %d\n", lpnrLocal->dwDisplayType);
        break;
    }

    printf("NETRESOURCE[%d] Usage: 0x%x = ", i, lpnrLocal->dwUsage);
    if (lpnrLocal->dwUsage & RESOURCEUSAGE_CONNECTABLE)
        printf("connectable ");
    if (lpnrLocal->dwUsage & RESOURCEUSAGE_CONTAINER)
        printf("container ");
    printf("\n");

    printf("NETRESOURCE[%d] Localname: %S\n", i, lpnrLocal->lpLocalName);
    printf("NETRESOURCE[%d] Remotename: %S\n", i, lpnrLocal->lpRemoteName);
    printf("NETRESOURCE[%d] Comment: %S\n", i, lpnrLocal->lpComment);
    printf("NETRESOURCE[%d] Provider: %S\n", i, lpnrLocal->lpProvider);
    printf("\n");
}

void main()
{
    printf("Getting the MAC Address: \n");
    GetMacAddress();
    
    printf("\nEnumerating Network Resources: \n");
    LPNETRESOURCE lpnr = NULL;

    if (EnumerateFunc(lpnr) == FALSE)
        printf("Call to EnumerateFunc failed\n");

    system("pause");
}