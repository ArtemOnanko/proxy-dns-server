/* 
 * File:   main.c
 * Author: artem
 *
 * Created on April 4, 2018, 2:40 PM
 */

#include "dns_header.h"

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
        int sockfd, numbytes_recv, numbytes_send, labellen, i=0;
	struct addrinfo hints;
	struct sockaddr_storage their_addr;
	unsigned char buf[PACKET_SIZE+4];
	char s[INET6_ADDRSTRLEN];                   // for printf
        socklen_t addr_len;
        
        unsigned char *ptr = malloc(PACKET_SIZE);
        unsigned char *name = malloc(PACKET_SIZE);
        
        
        load_init();
	dns_setup(hints, &sockfd);
                 
        for(;;)                                 // start dns iterative server
        {
            printf("listener: waiting to recvfrom...\n");
            numbytes_recv = recvfrom(sockfd, buf, PACKET_SIZE + 4, 0, (struct sockaddr *)&their_addr, &addr_len);
            printf("Received %i bytes of data from %s\n", numbytes_recv, inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s));
            printf("\n");
            
//            for(int j = 0 ; j < numbytes_recv; j++)
//            {
//                printf("%d ",buf[j]);
//            }
//            printf("\n");
            
            // Obtain addr from dns request in a simple way
                        
            ptr = buf + DNS_HEADER_LEN;
            while ((labellen = *ptr++))
            {
                  while (labellen--)
                  {
                      name[i++] = *ptr++;
                      
                      //printf("%c", *ptr++);
                  }
                  //printf(".");
                  name[i++] = '.';
            }
            name[--i] = 0;
            i = 0;
                
            printf("%s\n", name);
            // Here should be check if addr is in black list
            	            
            int blacklist = 1;
            if(!blacklist)
            {
                // проксируем запрос на сервер гугл
            }
            else
            {
                // host is not resolved  
                
                create_redirect_answer(buf, numbytes_recv);  
                
//                for(int j = 0 ; j < numbytes_recv+RESPONSE_SIZE; j++)
//                {
//                    printf("%d ",buf[j]);
//                }
//                printf("\n");
                
                if ((numbytes_send = sendto(sockfd, buf, (numbytes_recv+RESPONSE_SIZE), 0, (struct sockaddr *)&their_addr, addr_len)) == -1) 
                {
                    perror("talker: sendto");
                    exit(1);
                }
                           
               printf("talker: sent %d bytes to %s\n", numbytes_send, inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s));
            }
        }
	close(sockfd);
        free(ptr);
        free(name);
	return 0;
}

void dns_setup(struct addrinfo hints, int* sockfd)
{
        struct addrinfo *servinfo, *p;
        int rv;
        int yes = 1;            // // for loosing addr already in use message
        
        memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // set to AF_INET to force IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP     
        
	if ((rv = getaddrinfo(servaddr, MYPORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		exit(1);
	}
       
	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) 
        {
	    if ((*sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) 
            {
		perror("listener: socket");
		continue;
            }
                // loose the "Address already in use" message
            if (setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1)
            {
                perror("setsockopt");
                exit(1);
            }
            if (bind(*sockfd, p->ai_addr, p->ai_addrlen) == -1)
            {
		close(*sockfd);
		perror("listener: bind");
		continue;
            }
            break;
	}
        	
        if (p == NULL) 
        {
		fprintf(stderr, "listener: failed to bind socket\n");
                exit(1);
	}
	freeaddrinfo(servinfo);    
}

