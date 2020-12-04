#include "kasaSocketWin.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <thread>



int kasaSocketWin::kasaSetOn(const char* ipaddress) { return kasaControl(ipaddress, "ON"); }
int kasaSocketWin::kasaSetOff(const char* ipaddress) { return kasaControl(ipaddress, "OFF"); }
int kasaSocketWin::kasaQuery(const char* ipaddress) { return kasaControl(ipaddress, "QUERY"); }
int kasaSocketWin::kasaMeter(const char* ipaddress) { return kasaControl(ipaddress, "EMETER"); }



int kasaSocketWin::kasaDecrypt(const char *ptr)
    {
    uint8_t key = 171;
    uint8_t* p = buffer + 4;
    uint8_t q;

    for (int i = 4; i < msglen; i++)
        {
        q = buffer[i];
        *p = key ^ *p;
        key = q;
        p++;
        }
    *p = 0;
    return 0;
    }

int kasaSocketWin::kasaEncrypt(const char* msg)
    {
    uint8_t key = 171;
    uint8_t* p = buffer + 4;
    uint32_t x4;
    uint32_t x3;
    uint32_t x2;
    uint32_t x1;

    msglen = strlen(msg);

    x4 = msglen;
    x3 = x4 / 256;         x4 -= (256 * x3);
    x2 = x3 / 256;         x3 -= (256 * x2);
    x1 = x2 / 256;         x2 -= (256 * x1);

    buffer[0] = x1;
    buffer[1] = x2;
    buffer[2] = x3;
    buffer[3] = x4;

    memcpy(p, msg, msglen);
    msglen += 4;
    for (int i = 4; i < msglen; i++)
        {
        *p = key ^ *p;
        key = *p++;
        }
    return 0;
    }


int kasaSocketWin::kasaConnect(SOCKET* sck, const char *ipaddress, WSADATA *wsaData, struct sockaddr_in *inaddr)
    {
    int iResult;
    
    iResult = WSAStartup(MAKEWORD(2, 2), wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
        }

    *sck = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (*sck == INVALID_SOCKET)
        {
        wprintf(L"socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
        }

    inaddr->sin_family = AF_INET;


    iResult = inet_pton(AF_INET, ipaddress, &(inaddr->sin_addr.s_addr));

    inaddr->sin_port = htons(port);

    iResult = connect(*sck, (SOCKADDR*)inaddr, sizeof(*inaddr));
    if (iResult == SOCKET_ERROR) {
        wprintf(L"connect failed with error: %d\n", WSAGetLastError());
        closesocket(*sck);
        WSACleanup();
        return 1;
        }
    }

int kasaSocketWin::kasaControl(const char* ipaddress, const char* cmd)
    {
    const char* pcmd = NULL;
    uint8_t* p;


    if (strncmp(cmd, "ON", 2) == 0) pcmd = payload_on_raw;
    else
        {
        if (strncmp(cmd, "OFF", 3) == 0) pcmd = payload_off_raw;
        else
            {
            if (strncmp(cmd, "QUERY", 5) == 0) pcmd = payload_query_raw;
            else
                {
                if (strncmp(cmd, "EMETER", 6) == 0) pcmd = payload_emeter_raw;
                }
            }
        }
    if (pcmd != NULL)
        {
        SOCKET ConnectSocket = INVALID_SOCKET;
        struct sockaddr_in inaddr;
        WSADATA wsaData;
        int iResult;

        kasaEncrypt(pcmd);
        if (kasaConnect(&ConnectSocket, ipaddress, &wsaData, &inaddr) == 0)
            {

            iResult = send(ConnectSocket, (char*)buffer, msglen, 0);
            if (iResult == SOCKET_ERROR)
                {
                wprintf(L"send failed with error: %d\n", WSAGetLastError());
                closesocket(ConnectSocket);
                WSACleanup();
                return 1;
                }


            std::this_thread::sleep_for(std::chrono::microseconds(250000)); //give it 0.25 seconds to formulate response

            iResult = shutdown(ConnectSocket, SD_SEND);
            if (iResult == SOCKET_ERROR) {
                wprintf(L"shutdown failed with error: %d\n", WSAGetLastError());
                closesocket(ConnectSocket);
                WSACleanup();
                return 1;
                }

            // Receive until the server closes the connection
            p = buffer;
            msglen = 0;
            do {
                iResult = recv(ConnectSocket, (char*)p, 1024, 0);
                if (iResult > 0)
                    {
                    msglen += iResult;
                    p += iResult;//wprintf(L"Bytes received: %d\n", iResult);
                    }
                else
                    {
                    if (iResult < 0) wprintf(L"recv failed with error: %d\n", WSAGetLastError());
                    }
                } while (iResult > 0);

            if (msglen > 0) kasaDecrypt((char*)p);
            else *response = 0;

            iResult = closesocket(ConnectSocket);
            if (iResult == SOCKET_ERROR)
                {
                wprintf(L"close failed with error: %d\n", WSAGetLastError());
                WSACleanup();
                return 1;
                }
            }
        }
    //printf("%s\n", q);
    return 0;
    }

