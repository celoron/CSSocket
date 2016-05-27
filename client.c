#include "client.h"

int main(int argc, char *argv[]){
    Client* client;
    char buffer[256];
    char cmd[256];
    int server;

    client= connectToServer("localhost", 5000);

    server= client->fd;

    printf("Your id: %d\n", client->id);

    while(1){
        printf("> ");
        fgets(buffer,255,stdin);

        buffer[strlen(buffer)-1]= '\0';

        strcpy(cmd, buffer);

        char *save;
        char delim[] = " ";
        char* tok;

        tok= strtok_r(buffer, delim, &save);

        if(strcmp(tok, "exit")==0){
            break;
        }
        else if(strcmp(tok, "sendfile")==0){
            // to
            tok= strtok_r(NULL, delim, &save);
            int to= atoi(tok);

            // filename
            tok= strtok_r(NULL, delim, &save);
            strcpy(client->sendingFile, tok);

            sendPackageAndFree(server, actionPack(INCOMING_FILE, client->id, to));
        }
        else{
            sendPackageAndFree(server, strPack(cmd, client->id, SERVER));
        }

        client->responseWaiting= TRUE;

        while(client->responseWaiting==TRUE);

        if(client->lastResponse!=NULL){
            printf("%s\n", client->lastResponse);
        }

        free(client->lastResponse);
    }

    close(server);

    return 0;
}

Client* connectToServer(char *host, int port){
    int sockfd; // socket file desc

    struct sockaddr_in serverAdress;
    struct hostent *server;

    //soket file_desc kurulmasi
    //socket(domain, type, protocol)
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    //soketin basarili bir sekilde acilip acilmadigin kontrol et
    if (sockfd < 0)
        error("ERROR opening socket");

    //host adres bilgilerinin alinmasi
    server = gethostbyname(host);
    if (server == NULL)
        error("ERROR, no such host");

    //serverAdress temizleniyor
    bzero((char *) &serverAdress, sizeof(serverAdress));

    // serverAdress nin ayarlarinin yapilmasi
    serverAdress.sin_family = AF_INET; // internet domain/family (internet ipv4 icin AF_INET)
    serverAdress.sin_port = htons(port);  //htons byte order degistirmek icin kullanilir.
    bcopy((char *)server->h_addr, (char *)&serverAdress.sin_addr.s_addr, server->h_length); // server ip

    //servera baglanma
    if (connect(sockfd,(struct sockaddr *) &serverAdress,sizeof(serverAdress)) < 0)
        error("ERROR connecting");

    Client* client= malloc(sizeof(Client));
    client->fd= sockfd;

    //COMM_TYPE commType= getCommType(client->fd);
    //client->id= readInt(client->fd);

    // read client id
    Package* package= readPackage(client->fd);
    client->id= getInt(package);

    client->responseWaiting= FALSE;

    if(pthread_create( &client->thread, NULL, ClientThread, (void*) client)!=0){
        perror("Client thread başlatılamıyor");
    }

    return client;
}

void* ClientThread(void* _client){
    Client* client= (Client*)_client;

    int server= client->fd;

    int fileReceiving= FALSE;
    int sizeLeft= 0;
    char filename[255];

    Package* package;

    while(1){
        package= readPackage(server);

        if(package==NULL)return;

        if( package->header.to != client->id){
            debug("This package doesnt belong to me, its belongs to %d", package->header.to);
            continue;
        }

        if(package->header.type == STRING){
            char* str= getStr(package);

            client->lastResponse= str;
            client->responseWaiting= FALSE;
        }
        else if(package->header.type == ACTION){
            ActionType action= getAction(package);
            if(action == INCOMING_FILE){
                printf("[%d] wants to send file, accepting\n", package->header.from);

                sendPackageAndFree(server, actionPack(ACCEPT_FILE, client->id, package->header.from));
            }
            else if(action == ACCEPT_FILE){
                printf("[%d] accepted file transfer. Now sending file\n", package->header.from);

                client->lastResponse= NULL;
                client->responseWaiting= FALSE;
            }
            else if(action == CLIENT_NOT_EXIST){
                printf("Client doesnt exist! Cant send file!\n");

                client->lastResponse= NULL;
                client->responseWaiting= FALSE;
            }
        }
        else{
            debug("This package type is unknown: %d", package->header.type);
        }

        /*COMM_TYPE commType= getCommType(server);
        if(commType == INT){
            if (fileReceiving==TRUE){
                int size= readInt(server);
                sizeLeft= size;
            }
            else{
                int action= readInt(server);

                if(action== INCOMING_FILE){
                    // send accept action
                    writeInt(server, ACCEPT_FILE);
                    debug("Incoming file, accepting..");
                }
                if(action== ACCEPT_FILE){
                    // start sending file
                    struct stat st;
                    stat(client->sendingFile, &st);
                    int size = st.st_size;

                    writeInt(server, size);
                    writeStr(server, client->sendingFile);
                    debug("File accepted, sending size and filename");
                }
                else{
                    perror("Client thread unexpected action");
                }
            }

        }
        else if(commType == STRING){
            if (fileReceiving==TRUE){
                char* _filename= readStr(server);
                strcpy(filename, _filename);
                free(_filename);

                printf("File is going to receive %s, with sizeof %d\n", filename, sizeLeft);
                
            }
            else{
                client->lastResponse= readStr(server);
                client->responseWaiting= FALSE;
            }
        }
        else{
            //perror("Client thread unexpected comm type");
        }*/
    }
}