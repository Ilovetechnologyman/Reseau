
| Function | Parameters | Description |
|----------|------------|-------------|
| socket() | AF_INET, SOCK_STREAM, 0 | Creates TCP socket for IPv4 |
| bind() | sockfd, &addr, sizeof(addr) | Binds socket to address |
| listen() | sockfd, backlog | Sets socket to listen mode |
| accept() | sockfd, &client_addr, &addr_len | Accepts incoming connection |
| connect() | sockfd, &serv_addr, sizeof(serv_addr) | Connects to server |
| send() | sockfd, buffer, length, flags | Sends data |
| recv() | sockfd, buffer, length, flags | Receives data |