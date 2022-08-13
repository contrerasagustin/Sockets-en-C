// Client side C/C++ program to demonstrate Socket programming
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <iostream>
#define PORT 8080

using namespace std;

void help()
{
    cout << "Bienvenido al juego del ahoracado" << endl;
    cout << "Ingrese -h para ayuda" << endl;
    cout << "Para empezar a jugar necesita especificar los siguientes parametros: " << endl;
    cout << "-p <numero de puerto> | necesario para la ejecucion" << endl;
    cout << "-ip <numero de ip> | necesario para la ejecucion" << endl;
    cout << "La forma correcta de especificarlos es: " << endl;
    cout << "./cliente -ip <num_ip> -p <num_puerto>"<< endl;
    cout << "./cliente 8080 127.0.0.1" << endl;
}

bool isNumber(const string& str)
{
    for (char const &c : str) {
        if (std::isdigit(c) == 0) return false;
    }
    return true;
}

int validateParams(int argc, char *argv[], char *ipAdd)
{
    if (argc < 2)
    {
        cout << "La cantidad de parametros ingresados no es la suficiente, ante cualquier duda consulta la ayuda ['-h' o '--help']" << endl;
        exit(0);
    }
    
    if(strcmp(argv[1],"-h") == 0 || strcmp(argv[1],"--help") == 0){
        help();
        exit(1);
    }

    if(argc < 5) {
        cout << "La cantidad de parametros ingresados no es la correcta. Utiliza la ayuda ['-h' o '--help']" << endl;
        exit(0);
    }

    if(strcmp(argv[1], "-ip") == 0){
        strcpy(ipAdd, argv[2]);
    }

    if (strcmp(argv[3], "-ip") == 0){
            strcpy(ipAdd, argv[4]);
    }

    if(strcmp(argv[1], "-p") == 0){
        if(!isNumber(argv[2])){
            cout << "Port param is invalid. It must be a number of port" << endl;
            exit(0);
        }
        return atoi(argv[2]);
    }

    else{
        if(!isNumber(argv[4])){
            cout << "Port param is invalid. It must be a number of port" << endl;
            exit(0);
        }
        return atoi(argv[4]);
    }
}

int main(int argc, char *argv[])
{
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char letra;
    char leer[1024];
    char ipAdd[20];
    int op;
    string escribir;
    char buffer[1024] = {0};
    bool termino = false;

    system("clear");

    int port = validateParams(argc, argv, ipAdd);

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ipAdd, &serv_addr.sin_addr) <= 0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConexion Fallida\n");
        return -1;
    }

    read(sock, leer, 1024); // cliente conectado
    cout << leer << endl;
    read(sock, leer, 1024);
    cout << leer << endl;
    system("clear");
    while (!termino)
    {
        read(sock, leer, 1024);
        string sub="";
        sub=leer[0];
        sub=sub+leer[1];
        string letra;
        bool repetida=false;
        if (strcmp(sub.c_str(), "JU") == 0)
        {
            cout << "es tu turno de jugar" << endl;
            do{
                if(repetida)
                    cout << "la letra ingresada ya fue usada" << endl;
                repetida=false;
                cout << "ingresa una letra: ";
                cin >> letra;
                letra=toupper(letra[0]);
                for(int i = 2 ; i < strlen(leer);i++){
                    if(leer[i]==letra[0])
                        repetida=true;
                }
            }while(repetida);
            send(sock, letra.c_str(), letra.length(), 0);
            read(sock, leer, 1024);
            system("clear");
            cout << leer << "\n" << endl;
        }
        else if (strcmp(sub.c_str(), "ES") == 0)
        {
            cout << "espera tu turno para jugar" << endl;
            read(sock, leer, 1024);
            system("clear");
            cout << leer << "\n" << endl;
        }
        else
        {
            system("clear");
            cout << leer << endl;
            termino = true;
        }
    }
    return 0;
}