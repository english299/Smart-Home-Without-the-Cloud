#include "kasaSocket.h"

#include <cinttypes>
#include <stdlib.h>

#include <string>
#include <string.h>
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h> /* struct hostent, gethostbyname */
#include <unistd.h>

//uint16_t pPort = 999;


void error(const char* msg) { perror(msg); exit(0); }


int kasaSocket::kasaSetOn(const char* ipaddress) { return kasaControl(ipaddress, "ON"); }
int kasaSocket::kasaSetOff(const char* ipaddress) { return kasaControl(ipaddress, "OFF"); }
int kasaSocket::kasaQuery(const char* ipaddress) { return kasaControl(ipaddress, "QUERY"); }
int kasaSocket::kasaMeter(const char* ipaddress) { return kasaControl(ipaddress, "EMETER"); }


int kasaSocket::kasaDecrypt(const char* ptr)
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

int kasaSocket::kasaEncrypt(const char* msg)
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



int kasaSocket::kasaControl(const char* ipaddress, const char* cmd)
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
        struct hostent* server;
        struct sockaddr_in serv_addr;
        int sockfd, bytes, sent, received, total;

        kasaEncrypt(pcmd);
        if (kasaConnect(&sockfd,ipaddress,server,&serv_addr ) == 0)
            {
            total = msglen;
            sent = 0;
            do 
                {
                bytes = write(sockfd, buffer + sent, total - sent);
                if (bytes < 0)
                    error("ERROR writing message to socket");
                if (bytes == 0)
                    break;
                sent += bytes;
                } while (sent < total);
                
            usleep(250000);

            memset(buffer, 0, 1024);
            total = 1023;
            msglen = 0;
            char* p, * q;

            timeval tv;
            tv.tv_sec = 1;
            tv.tv_usec = 0;

            setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

            do
                {
                bytes = read(sockfd, buffer + msglen, total - msglen);
                if (bytes <= 0)
                        break;
                msglen += bytes;

                } while (msglen < total);




            if (msglen == total)
                error("ERROR storing complete response from socket\n");


            if (msglen > 0) kasaDecrypt(p);
            else *response = 0;

            close(sockfd);

            }
        }
    }



int kasaSocket::kasaConnect(int *sockfd, const char* ipaddress, hostent* server, struct sockaddr_in* inaddr)
    {
    /* create the socket */
    *sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (*sockfd < 0) error("ERROR opening socket");

    /* lookup the ip address */
    server = gethostbyname(ipaddress);
    if (server == NULL) error("ERROR, no such host");

    /* fill in the structure */
    memset(inaddr, 0, sizeof(*inaddr));
    inaddr->sin_family = AF_INET;
    inaddr->sin_port = htons(port);
    memcpy(&(inaddr->sin_addr.s_addr), server->h_addr, server->h_length);

    /* connect the socket */
    if (connect(*sockfd, (struct sockaddr*)inaddr, sizeof(*inaddr)) < 0)
        error("ERROR connecting");
    return 0;
    }


