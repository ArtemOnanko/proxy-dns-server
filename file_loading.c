/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "dns_header.h"

FILE*  init;

unsigned char response[] = {0xc0, 0x0c, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x02, 0x58, 0x00, 0x04, 0, 0, 0, 0}; // 4 last bytes for redirecting ip addr

void load_init(void)
{
    int i,j = 0, counter = 4;
    char xxx[3];    
    init = fopen("server.txt", "r");
    for(i = 0; servaddr[i] = fgetc(init), servaddr[i] != '\n'; i++)     // parse servaddr
    {
    }
    servaddr[i] = '\0';
    for(i = 0; upservaddr[i] = fgetc(init), upservaddr[i] != '\n'; i++) // parse upservaddr
    {
    }
    upservaddr[i] = '\0';
    for(i = 0; localaddr[i] = fgetc(init), localaddr[i] != '\n'; i++)   // parse localaddr and insert it to response[]
    {   
        if(localaddr[i] != '.')
        {
            xxx[j++] = localaddr[i];
        }
        else
        {
            response[RESPONSE_SIZE - counter] = atoi(xxx);
            counter--;
            for(j = 0; j < sizeof(xxx); j++)
            {
                xxx[j] = 0;
            }
            j = 0;
        }   
    }
    response[RESPONSE_SIZE - counter] = atoi(xxx);
    localaddr[i] = '\0';
    
    printf("Proxy server IP is %s\nUpper level dns server IP is %s\nIP address for redirecting %s\n", servaddr, upservaddr, localaddr); 
}

void create_redirect_answer(unsigned char buf[], int numbytes)
{
    buf[2] = buf[2]|0x84;      // answer, authoritative
    buf[3] = 0x00;             // error 5 (or 0 for local address)
    buf[7] = 1;                // one respond
   
    memcpy(&buf[numbytes], response, RESPONSE_SIZE);
}