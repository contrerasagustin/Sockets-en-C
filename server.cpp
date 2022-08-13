#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <iostream>
#include <thread>
#include <signal.h>
#include <fstream>
#include <fcntl.h>
#include <semaphore.h>
#include <string>
#include <mutex>
#include <vector>
#include <list>
#define PORT 60000
#define TAM 1024
#define TEXTO "AHORCADO";

using namespace std;

int jugador[3];
int puntaje[3] = {0, 0, 0};
list<int> jugadores;
int cantJugadores = 0;
int i;
int turno = 0;
mutex mutexJugador[3];
string palabra;
string faltan;
string encotrada;
int cantLetras;
string letraUsadas = "";
thread th[3];
int vidas = 8;
sem_t semInicio;
sem_t semLeer;
sem_t semFin;
bool termino = false;
string ultLetraIng;
string resMsj;
string resultadoPartida;
fd_set descriptorLectura;
bool timeout = false;
bool iniciar = true;

string palabraRandom(string path)
{
    int lineas = 0, i = 0;
    string pal;
    vector<string> v;
    ifstream reader(path);
    if (reader.is_open())
    {
        while (std::getline(reader, pal))
            v.push_back(pal);
        srand(time(NULL));
        int random = rand() % v.size();
        pal = v.at(random);
        reader.close();
    }

    while (i < pal.length())
    {
        if (islower(pal[i]))
        {
            pal[i] = toupper(pal[i]);
        }
        i++;
    }
    return pal;
}

void help()
{
    cout << "Bienvenido al juego del ahoracado" << endl;
    cout << "Ingrese -h para ayuda" << endl;
    cout << "El juego permite hasta 3 jugadores y jugarse en red" << endl;
    cout << "Para ello necesita especificar los siguientes parametros: " << endl;
    cout << "<numero de puerto> | necesario para la ejecucion" << endl;
    cout << "La forma correcta de especificarlos es: " << endl;
    cout << "./server <num_puerto>" << endl;
    cout << "./server 8080" << endl;
}

bool esUnNumero(const string &str)
{
    for (char const &c : str)
    {
        if (isdigit(c) == 0)
            return false;
    }
    return true;
}

int validateParams(int argc, char *argv[])
{
    if (argc != 2)
    {
        cout << "La cantidad de parametros ingresados no es la correcta, ante cualquier duda consulta la ayuda ['-h' o '--help']" << endl;
        exit(0);
    }
    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
    {
        help();
        exit(1);
    }
    if (!esUnNumero(argv[1]))
    {
        cout << "Parametro no valido, tiene que ser un numero" << endl;
        exit(0);
    }
    if (atoi(argv[1]) <= 8079)
    {
        cout << "El puerto tiene que ser igual o superior al 8080" << endl;
        exit(0);
    }
    return atoi(argv[1]);
}

string mensaje(string condicion)
{
    string msj = "\n\tjugador: " + to_string(turno + 1);
    msj = msj + "\n\n\tletra ingresada: " + ultLetraIng;
    msj = msj + "\n\n\tcondicion: " + condicion;
    msj = msj + "\n\n\tpalabra: ";
    for (int i = 0; i < palabra.length(); i++)
    {
        msj = msj + encotrada[i] + " ";
    }
    msj = msj + "\n\n\tletras anteriores: ";
    for (int i = 0; i < letraUsadas.length(); i++)
    {
        msj = msj + letraUsadas[i] + " ";
    }
    msj = msj + "\n\n\tintentos restante: " + to_string(vidas);
    return msj;
}

string final()
{
    string msj = "";
    msj = msj + "\n\tpartida terminada";
    msj = msj + "\n\n\tresultado de la partida: " + resultadoPartida;
    msj = msj + "\n\n\tresultados de los competidores";
    for (int i = 0; i < cantJugadores; i++)
    {
        msj = msj + "\n\n\tjugador " + to_string(i + 1) + ": " + to_string(puntaje[i]) + " puntos";
    }
    return msj;
}

int siguiente(int j)
{
    if (j + 1 == cantJugadores)
        return 0;
    else
        return j + 1;
}

bool contiene(char l)
{
    for (int i = 0; i < letraUsadas.length(); i++)
    {
        if (l == letraUsadas[i])
            return true;
    }
    return false;
}

int ahorcado(char l, int j)
{
    bool acierto = false;
    string aux1 = "";
    string aux2 = "";
    if (contiene(l))
        return 4;
    for (int i = 0; i < faltan.length(); i++)
    {
        if (faltan[i] == l)
        {
            aux1 = aux1 + "_";
            aux2 = aux2 + l;
            cantLetras--;
            acierto = true;
            puntaje[j] = puntaje[j] + 2;
        }
        else
        {
            aux1 = aux1 + faltan[i];
            aux2 = aux2 + encotrada[i];
        }
    }
    encotrada = aux2;
    faltan = aux1;
    if (acierto)
    {
        if (cantLetras == 0)
        {
            resultadoPartida = "GANADA";
            return 0; // gano
        }
        return 1; // acerto
    }
    vidas--;
    puntaje[j] = puntaje[j] - 1;
    if (vidas == 0)
    {
        resultadoPartida = "PERDIDA";
        return 2; // perdio
    }
    else
        return 3; // error
}

void jugar(int socket, int j)
{
    char leer[2];
    char escribir[1024] = "conectado al servidor\nesperando a los demas jugadores";
    char op;
    bool cortar = true;
    int rc = send(socket, escribir, 1024, 0);
    sleep(1);
    sem_wait(&semInicio);
    strcpy(escribir, "incio el juego, espera tu turno");
    rc = send(socket, escribir, 1024, 0);

    sleep(1);
    while (cortar)
    {
        if (turno == j)
        {
            if (termino)
            {
                rc = send(socket, final().c_str(), 1024, 0);
                if(rc==-1)
                    break;
                cortar = false;
                system("clear");
                cout << final() << endl;
            }
            else
            {
                // 0 - termino
                // 1 - jugar
                // 2 - esperar
                strcpy(escribir, "JU");
                strcat(escribir, letraUsadas.c_str());
                rc = send(socket, escribir, 1024, 0);
                if(rc==-1)
                    break;
                read(socket, leer, 2);
                ultLetraIng = leer[0];
                int res = ahorcado(ultLetraIng[0], j);
                strcpy(escribir, encotrada.c_str());
                switch (res)
                {
                case 1:
                    // strcat(escribir, "\nacertaste una letra");
                    resMsj = mensaje("acertada");
                    rc = send(socket, resMsj.c_str(), 1024, 0);
                    break;
                case 2:
                    // strcat(escribir, "\nperdiste");
                    resMsj = mensaje("errada");
                    rc = send(socket, resMsj.c_str(), 1024, 0);
                    termino = true;
                    break;
                case 0:
                    // strcat(escribir, "\nganaste");
                    resMsj = mensaje("acertada");
                    rc = send(socket, resMsj.c_str(), 1024, 0);
                    termino = true;
                    break;
                case 3:
                    // strcat(escribir, "\nla letra ingresada no es valida");
                    resMsj = mensaje("errada");
                    rc = send(socket, resMsj.c_str(), 1024, 0);
                    break;
                }
                if(rc==-1)
                    break;
                turno = siguiente(j);
                system("clear");
                cout << resMsj << endl;
                for (int i = 0; i < cantJugadores - 1; i++)
                {
                    sem_post(&semLeer);
                }
                sleep(1);
                letraUsadas = letraUsadas + ultLetraIng;
            }
        }
        else
        {
            if (!termino)
            {
                strcpy(escribir, "ES");
                strcat(escribir, letraUsadas.c_str());
                rc = send(socket, escribir, 1024, 0);
                if(rc == -1)
                    break;
                sem_wait(&semLeer);
                rc = send(socket, resMsj.c_str(), 1024, 0);
                if(rc == -1)
                    break;
                sleep(1);
            }
            else
            {
                cout << final() << endl;
                cortar = false;
                rc = send(socket, final().c_str(), 1024, 0);
                if(rc==-1)
                    break;
                system("clear");
                cout << final() << endl;
            }
        }
    }
    sem_post(&semFin);
    if (rc != -1 )
        close(socket);
    mutexJugador[j].unlock();
}

void comenzar(int server_fd)
{
    cout << encotrada << endl;
    for (int i = 0; i < palabra.length(); i++)
    {
        encotrada = encotrada + "_";
    }
    for (i = 0; i < cantJugadores; i++)
    {
        sem_post(&semInicio);
    }
    for (i = 0; i < cantJugadores; i++)
    {
        sem_wait(&semFin);
    }
    cantJugadores = 0;
    puntaje[0] = 0;
    puntaje[1] = 0;
    puntaje[2] = 0;
    turno = 0;
    letraUsadas = "";
    vidas = 8;
    termino = false;
    timeout = false;
    encotrada="";
    sleep(5);
    system("clear");
    for (i = 0; i < 3; i++)
        FD_CLR(jugador[i], &descriptorLectura);
    cout << "servidor de juego reiniciado" << endl;
    for (i = 0; i < 3; i++)
        FD_SET(jugador[i], &descriptorLectura);
    palabra = palabraRandom("./palabras.txt");
    faltan = palabra;
    cantLetras = palabra.length();

    cout << "la palabra a adivinar va a ser: " << palabra << endl;
}

void esperarJugadores(int server_fd)
{
    sleep(5);
    timeout = true;
    comenzar(server_fd);
}

void manejador (int signum){
    printf("\nRecibi la señal sigint\n");
    signal(SIGINT,manejador);   
}

void chauCliente (int signum){
    printf("\nSe desconecto un cliente abruptamente, se cerraran todas las conexiones\n");
    for (int i = 0; i < 3; i++)
    {
        close(jugador[i]);
    }
    signal(SIGINT,chauCliente);   
}

int main(int argc, char *argv[])
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    sem_init(&semInicio, 0, 0);
    sem_init(&semLeer, 0, 0);
    sem_init(&semFin, 0, 0);
    FD_ZERO(&descriptorLectura);
    signal(SIGINT,manejador);
    signal(SIGPIPE,chauCliente);
    system("clear");

    int port = validateParams(argc, argv);
    //int port = 8080;
    cout << "servidor de juego iniciado" << endl;
    cout << "en el puerto: " << port << endl;

    palabra = palabraRandom("./palabras.txt");
    faltan = palabra;
    cantLetras = palabra.length();

    cout << "la palabra a adivinar va a ser: " << palabra << endl;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 0) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    int jug;
    FD_SET(server_fd, &descriptorLectura);
    for (i = 0; i < 3; i++)
        FD_SET(jugador[i], &descriptorLectura);
    
    while (true)
    {
        select(4, &descriptorLectura, NULL, NULL, NULL);
        if (FD_ISSET(server_fd, &descriptorLectura))
        {
            /* Un nuevo cliente solicita conexión. Aceptarla aquí. En el ejemplo, se acepta la conexión, se mete el descriptor en socketCliente[] y se envía al cliente su posición en el array como número de cliente. */
            if (!timeout && cantJugadores < 3)
            {
                mutexJugador[cantJugadores].lock();
                jug = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
                if (jug < 0)
                {
                    perror("accept");
                    exit(EXIT_FAILURE);
                }
                if (timeout)
                {
                    close(jug);
                }
                else
                {
                    jugador[cantJugadores] = jug;
                    cout << "se conecto el jugador " << cantJugadores + 1 << endl;
                    thread(jugar, jugador[cantJugadores], cantJugadores).detach();
                    cantJugadores++;
                    if (cantJugadores == 1)
                    {
                        thread(esperarJugadores,server_fd).detach();
                    }
                }
            }
        }
        
        //if (timeout && iniciar)
        //{
        //    thread(comenzar).detach();
        //}
        /* Se tratan los clientes */
    }
    return 0;
}