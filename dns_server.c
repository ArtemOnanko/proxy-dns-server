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
        int sockfd, numbytes;
	struct addrinfo hints;
	struct sockaddr_storage their_addr;
	unsigned char buf[PACKET_SIZE+4];
	char s[INET6_ADDRSTRLEN];                   // for printf
        socklen_t addr_len;
                  
        load_init();
	dns_setup(hints, &sockfd);
                 
        for(;;)                                 // start dns iterative server
        {
            printf("listener: waiting to recvfrom...\n");
            numbytes = recvfrom(sockfd, buf, PACKET_SIZE + 4, 0, (struct sockaddr *)&their_addr, &addr_len);
            printf("Received %i bytes of data from %s\n", numbytes, inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s));
            printf("\n");
            for(int j = 0 ; j < numbytes; j++)
            {
                printf("%d ",buf[j]);
            }
            printf("\n");
            	            
            int blacklist = 1;
            if(!blacklist)
            {
                // проксируем запрос на сервер гугл
            }
            else
            {
                // host is not resolved  
                
                create_redirect_answer(buf, numbytes);
            
                for(int i = 0 ; i < numbytes+RESPONSE_SIZE; i++)
                {
                    printf("%d ",buf[i]);
                }           
                
                if ((numbytes = sendto(sockfd, buf, (numbytes+RESPONSE_SIZE), 0, (struct sockaddr *)&their_addr, addr_len)) == -1) 
                {
                    perror("talker: sendto");
                    exit(1);
                }
                           
               printf("talker: sent %d bytes to %s\n", numbytes, inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s));
            }
        }
	close(sockfd);
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

