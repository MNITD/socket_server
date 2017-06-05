#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>
#include <sstream>
#include <map>

void error(const std::string &m) {
    std::string message("[server] ERROR " + m);
    perror(message.c_str());
    exit(1);
}

void log(const std::string &m) {
    printf("[server] %s \n", m.c_str());
}

std::vector<std::string> split(const std::string &line) {
    // std::operator>> separates by spaces
    std::vector<std::string> words;
    std::string word;
    std::istringstream split(line);

    while (split >> word) {
        words.push_back(word);
    }

    return words;
}

std::string concat(const std::vector<std::string> &v, int start, int end) {
    std::string result;
    int i = start;
    while (i < end) {
        result += v.at(i);
        result += " ";
        ++i;
    }
    return result;
}

std::string get_current_time() {
    time_t now = time(0);
    tm *ltm = localtime(&now);
    std::string time;
    time += std::to_string(ltm->tm_hour);
    time += ":";
    time += std::to_string(ltm->tm_min);
    time += ":";
    time += std::to_string(ltm->tm_sec);
    return time;
}

std::string get_current_date() {
    time_t now = time(0);
    tm *ltm = localtime(&now);
    std::string date;
    date += std::to_string(ltm->tm_mday);
    date += ".";
    date += std::to_string(1 + ltm->tm_mon);
    date += ".";
    date += std::to_string(1900 + ltm->tm_year);
    return date;
}

int main(int argc, char *argv[]) {
    int sockfd, newsockfd, portno, pid;
    socklen_t clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    std::vector<std::string> received;
    std::map<std::string, int> ops_map = {{"t", 0},
                                          {"d", 1},
                                          {"h", 2},
                                          {"m", 3}};
    std::string response;

    /* Create socket file descriptor */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
        error("opening socket");
    else
        log("Has opened socket");


    /* Initialize socket structure */
    bzero((char *) &serv_addr, sizeof(serv_addr));

    /* Getting portno from args*/
//    if(argc > 1)
//        portno = atoi(argv[1]); // converts string to int
//    else
    portno = 5002; // default port


    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    /* Bind the host address with file descriptor.*/
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("on binding");
    else
        log("Has bind host address");


    /* Now start listening for the clients, here process will
       * go in sleep mode and will wait for the incoming connection
       * place all incoming connection into a backlog queue
       * until accept() call accepts the connection.
       * set the maximum size for the backlog queue to 5
    */
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    while (true) {
        /* Accept actual connection from the client
            * returns a new socket file descriptor for the accepted connection.
            * the original socket file descriptor can continue to be used
         */
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

        if (newsockfd < 0) {
            error("on accept");
        }

        char str[INET_ADDRSTRLEN];
        printf("[server] got connection from %s port %d\n",
               inet_ntop(AF_INET, &(cli_addr.sin_addr), str, INET_ADDRSTRLEN), portno);

        pid = fork();

        if(pid < 0){
            error("on fork");
        } else if(pid != 0){ //client process
            close(newsockfd);
        } else { //parent process
            close(sockfd);
            /* If connection is established then start communicating */
            bzero(buffer, 256);

            if (read(newsockfd, buffer, 255) < 0) {
                error("reading from socket");
            }

            received = split(buffer);

        // for(int i = 0; i < received.size(); i++)
            switch (ops_map[received.at(0)]) {
                case 0:
                    response = get_current_time();
                    break;
                case 1:
                    response = get_current_date();
                    break;
                case 2:
                    response = "Aloha";
                    break;
                case 3:
                    response = concat(received, 1, received.size());
                    break;
                default:
                    break;
            }
        //  printf("Here is the message: %s\n",buffer);

            /* Write a response to the client
                * equal to send() without any flags as last arg
             */

            if (write(newsockfd, response.c_str(), response.size()) < 0) {
                error("writing to socket");
            }
            return 0;
        }
    }
}