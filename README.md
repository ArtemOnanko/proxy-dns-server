# proxy-dns-server
Simple proxy dns server with black list support

How to use:
1. Insert into line 1 of server.txt the IPv4 address of our proxy dns server. (Use this address as dns in your network settings)
2. Insert to the line 2 of server.txt the IPv4 address of dns server we will forward request to. 
3. Insert IPv4 address you want to use for redirecting to the line 3 of server.txt
4. Starting from the line 4 put the domain names you wish to add to the black list, one name per line.
Allowed symbols are latin letters (upper/lower case), numbers, slash sign, dot sign.
Compile using makefile, run with sudo.

How it works:
On the begining the program load server.txt to obtain requred IP addresses and to create a trie structure 
for domain names you wish to add to the black list.
Then the iterative udp server start listening on port 53 for incoming dns requests.
When udp packet with dns request comes, the server parse it to obtain required domain name.
Then the server start looking for this name in the trie structure.
If the name is in black list then the server send back modified udp packet which redirects sender to the ip address from the line 3 of the server.txt
If the name is absent in the black list then the server creates raw udp socket and send modified udp packet to the dns server from 
the line 2 of server.txt. After this, dns server will answer directly to the sender but not to our proxy server. In such way 
our proxy forwards the dns requests.
