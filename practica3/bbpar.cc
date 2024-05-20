#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <mpi.h>
#include "libbb.h"

using namespace std;

unsigned int NCIUDADES;
int rank, size;

//Etiquetas para el equilibrado de carga
const int PETICION = 0, 
          TRABAJO = 1,
          TESTIGO_DETECCION = 2,
          FIN_DETECTADO = 3;

//Etiquetas para la detección de fin
const int MENSAJE_TRABAJO = 0,
          MENSAJE_PETICION = 1,
          MENSAJE_TOKEN = 2,
          TRABAJO_AGOTADO = 3;

//Etiquetas para la difusión de cota superior
const int ENVIO_CS = 0;

//Flag para el estado del proceso
const int PASIVO = 0,
          ACTIVO = 1;

//Flag para el color del proceso
const int NEGRO = 0,
          BLANCO = 1;

int estado, color, token;
bool token_presente;

void Difusion_Cota_Superior(int id_Proceso, int size, int cs) {
    bool difundir_cs_local = true;
    bool pendiente_retorno_cs = false;
    int cs_local = cs;
    int siguiente = id_Proceso + 1;
    int flag;
    MPI_Status status;
    int anterior/* = id_Proceso == 0 ? size - 1 : id_Proceso - 1*/;

    if (id_Proceso == 0){
        anterior = size - 1;
    }
    else {
        anterior = id_Proceso - 1;
    }
    if (difundir_cs_local && !pendiente_retorno_cs){
        // Enviar valor local de cs al proceso(id + 1) % P;
        MPI_Send(
            &cs_local,          //ID de proceso que envia
            1,                  //Numero de elementos enviados
            MPI_INT,            //Tipo de mensaje
            siguiente % size,   //Destinatario del mensaje
            ENVIO_CS,           // Tag de peticion de trabajo
            MPI_COMM_WORLD      //Comunicador por el que se envia
        );
        pendiente_retorno_cs = true;
        difundir_cs_local = false;
    }
    // Sondear si hay mensajes de cota superior pendientes;

    MPI_Iprobe(
        MPI_ANY_SOURCE,         //ID del proceso que ha enviado mensaje
        ENVIO_CS,               //Tag con el que se espera recibir el mensaje
        MPI_COMM_WORLD,         //Comunicador utilizado
        &flag,                  //Flag del probe
        &status                 //MPI_Status del iprobe
    );
    while (flag)
    {
        // Recibir mensaje con valor de cota superior desde el proceso(id - 1 + P) % P;    
        MPI_Recv(
            &cs_local,                        // Donde recibe
            1,                                // Tamaño del mensaje
            MPI_INT,                          // Tipo de dato del mensaje
            anterior % size,                  // De donde espera recibir
            status.MPI_TAG,                   // Tag
            MPI_COMM_WORLD,                   // Comunicador global
            &status                           // Estado de receive
        );

        // Actualizar valor local de cota superior;
        if (status.MPI_SOURCE == id_Proceso && difundir_cs_local) {
            // Enviar valor local de cs al proceso(id + 1) % P;
            MPI_Send(
                &cs_local,          //ID de proceso que envia
                1,                  //Numero de elementos enviados
                MPI_INT,            //Tipo de mensaje
                siguiente % size,   //Destinatario del mensaje 
                ENVIO_CS,           // Tag de peticion de trabajo
                MPI_COMM_WORLD      //Comunicador por el que se envia
            );
            pendiente_retorno_cs = true;
            difundir_cs_local = false;
        }
        else if (status.MPI_SOURCE == id_Proceso && !difundir_cs_local) {
            pendiente_retorno_cs = false;
        }
        else{ // origen mensaje == otro proceso
            // Reenviar mensaje al proceso(id + 1) % P;
            MPI_Send(
                &cs_local,          //ID de proceso que envia
                1,                  //Numero de elementos enviados
                MPI_INT,            //Tipo de mensaje
                siguiente % size,   //Destinatario del mensaje 
                ENVIO_CS,           //Tag de envío de cota superior
                MPI_COMM_WORLD      //Comunicador por el que se envia
            );
        }
        // Sondear si hay mensajes de cota superior pendientes;
        MPI_Iprobe(
            MPI_ANY_SOURCE,         //ID del proceso que ha enviado mensaje
            ENVIO_CS,               //Tag con el que se espera recibir el mensaje
            MPI_COMM_WORLD,         //Comunicador utilizado
            &flag,                  //Flag del iprobe
            &status                 //MPI_Status del iprobe
        );
    }
}

void Dijkstra (int id_Proceso, int size, int &estado, int &color, int &token, bool &token_presente) {
    // P(id)
    //     ...Esperar evento;
    
    MPI_Status status;
    int flag;
    int siguiente = id_Proceso + 1;

    int anterior/* = id_Proceso == 0 ? size - 1 : id_Proceso - 1*/;

    if (id_Proceso == 0) {
        anterior = size - 1;
    }
    else {
        anterior = id_Proceso - 1;
    }

    MPI_Iprobe(
        MPI_ANY_SOURCE, // De donde espera recibir el mensaje
        MPI_ANY_TAG,    // Espera cualquier tag
        MPI_COMM_WORLD, // Comunicador global
        &flag,
        &status         // Estado del probe
    );
    
    switch (status.MPI_TAG){
        case MENSAJE_TRABAJO:
    //      estado = ACTIVO;
            estado = ACTIVO;
            break;

        case MENSAJE_PETICION:
//            if (hay trabajo para ceder)
            if (true) //Falta la condicion
            {
//                j = origen(PETICION);
                int j = status.MPI_SOURCE;
//                Enviar TRABAJO a P(j);
                MPI_Send(
                    &id_Proceso,      //Información enviada
                    1,                //Numero de elementos enviados
                    MPI_INT,          //Tipo de mensaje
                    j,                //Destinatario del mensaje 
                    MENSAJE_TRABAJO,  // Tag
                    MPI_COMM_WORLD    //Comunicador por el que se envia
                );
//                if (id < j){
//                    mi_color = NEGRO;
//                }
                if (id_Proceso < j) {
                    color = NEGRO;
                }
            }
            break;

        case MENSAJE_TOKEN:
//            token_presente = TRUE;
            token_presente = true;

//            if (estado == PASIVO)
            if (estado == PASIVO) {
                if (id_Proceso == 0 && color == BLANCO && token == BLANCO){
//                    TERMINACION DETECTADA;
                }
                else {
                    if (id_Proceso == 0) { // if (id == 0)
                        // color(TOKEN) = BLANCO;
                        token = BLANCO;
                    }
                    else if (color == NEGRO) {   //else if (mi_color == NEGRO)
//                        color(TOKEN) = NEGRO;
                        token = NEGRO;
                    }

//                    Enviar TOKEN a P(id - 1);
                    MPI_Send(
                        &token,              //Información enviada
                        1,                   //Numero de elementos enviados
                        MPI_INT,             //Tipo de mensaje
                        anterior,            //Destinatario del mensaje
                        MENSAJE_TOKEN,       // Tag de peticion de trabajo
                        MPI_COMM_WORLD       //Comunicador por el que se envia
                    );
//                    mi_color = BLANCO;
                    color = BLANCO;
//                    token_presente = FALSE;
                    token_presente = false;
                }
            }
            break;
        
        case TRABAJO_AGOTADO:
//            estado = PASIVO;
            estado = PASIVO;
            if (token_presente) {
                if (id_Proceso == 0){//if (id == 0)
                //color(TOKEN) = BLANCO;
                    token = BLANCO;
                }  
                else if (color == NEGRO) {
                    //color(TOKEN) = NEGRO;
                    token = NEGRO;
                }
                //Enviar TOKEN a P(id - 1);
                MPI_Send(
                    &token,              //Información enviada
                    1,                   //Numero de elementos enviados
                    MPI_INT,             //Tipo de mensaje
                    anterior,            //Destinatario del mensaje
                    MENSAJE_TOKEN,       // Tag de peticion de trabajo
                    MPI_COMM_WORLD       //Comunicador por el que se envia
                );
                //mi_color = BLANCO;
                color = BLANCO;
                //token_presente = FALSE;
                token_presente = false;
            }
            break;
    }
}

void Equilibrado_Carga(int id_Proceso, int size, tPila * pila, bool *fin)
{
    cout << "valor(fin) => " << *fin << endl;
    int anterior/* = id_Proceso == 0 ? size - 1 : id_Proceso - 1*/;
    int flag;
    MPI_Status status;

    int peticion_trabajo; // id de quien lo solicita
    if (id_Proceso == 0){
        anterior = size - 1;
    }
    else {
        anterior = id_Proceso - 1;
    }

    int siguiente = id_Proceso + 1;
    if (pila->vacia()){ 
        // cout << "pila->vacia() antes de send || id_Proceso => " << id_Proceso << " siguiente => " << siguiente % size << endl;
        // el proceso no tiene trabajo: pide a otros procesos
        // Enviar peticion de trabajo al proceso(id + 1) % P;
        MPI_Send(
            &id_Proceso,         //Información enviada
            1,                   //Numero de elementos enviados
            MPI_INT,             //Tipo de mensaje
            siguiente % size,    //Destinatario del mensaje
            PETICION,            // Tag de peticion de trabajo
            MPI_COMM_WORLD       //Comunicador por el que se envia
        );
        // cout << "pila->vacia() despues de send || id_Proceso => " << id_Proceso << " siguiente => " << siguiente % size << endl;

        while (pila->vacia() && !(*fin))
        {
            // cout << "while (pila->vacia() && !fin) antes de probe" << endl;
            // Esperar mensaje de otro proceso;
            MPI_Probe(
                MPI_ANY_SOURCE, // De donde espera recibir el mensaje
                MPI_ANY_TAG,    // Espera cualquier tag
                MPI_COMM_WORLD, // Comunicador global
                &status         // Estado del probe
            );
            // cout << "while (pila->vacia() && !fin) despues de probe" << endl;

            switch (status.MPI_TAG){
                case PETICION: // Si es petición de trabajo (0)
                    // peticion de trabajo
                    // Recibir mensaje de peticion de trabajo;
                    cout << "PETICION => source == " << status.MPI_SOURCE << endl << flush;
                    MPI_Recv(
                        &peticion_trabajo,       // Donde recibe
                        1,                       // Tamaño del mensaje
                        MPI_INT,                 // Tipo de dato del mensaje
                        status.MPI_SOURCE,       // De donde espera recibir
                        PETICION,                // Tag
                        MPI_COMM_WORLD,          // Comunicador global
                        &status                  // Estado de receive
                    );

                    if (status.MPI_ERROR == MPI_SUCCESS) {
                        cout << "Recv PETICION correcto " << endl << flush;
                    }
                    else {
                        cout << "Recv PETICION incorrecto" << endl << flush;
                    }
                    if (status.MPI_SOURCE == id_Proceso) // solicitante = id
                    {
                        cout << "\t\t\t\t\t" << "MPISOURCE == " << id_Proceso << endl << flush;
                        // peticion devuelta
                        // Reenviar peticion de trabajo al proceso(id + 1) % P;
                        MPI_Send(
                            &id_Proceso,      //Información enviada
                            1,                //Numero de elementos enviados
                            MPI_INT,          //Tipo de mensaje
                            siguiente % size, //Destinatario del mensaje 
                            PETICION,         // Tag
                            MPI_COMM_WORLD    //Comunicador por el que se envia
                        );
                        // Iniciar deteccion de posible situacion de fin;
                        
                        //Cuando esté correcto el equilibrado de carga probar
                        //con la deteccion de fin
                        //Dijkstra(id_Proceso, size, estado, color, token, token_presente);
                    }
                    else {
                        // cout << "else" << endl;
                        cout << "\t\t\t\t\t" << "(else)MPISOURCE == " << id_Proceso << endl << flush;
                        // peticion de otro proceso: la retransmite al siguiente
                        // Pasar peticion de trabajo al proceso(id + 1) % P;
                        MPI_Send(
                            &id_Proceso,         //Información enviada
                            1,                   //Numero de elementos enviados
                            MPI_INT,             //Tipo de mensaje
                            siguiente % size,    //Destinatario del mensaje 
                            PETICION,            // Tag
                            MPI_COMM_WORLD       //Comunicador por el que se envia
                        );
                    }

                    break;
                case TRABAJO: // Si recibe nodos (1)
                    // resultado de una peticion de trabajo
                    // Recibir nodos del proceso donante;
                    cout << "TRABAJO" << endl;
                    MPI_Recv(
                        &peticion_trabajo,      // Donde recibe
                        status._ucount,                      // Tamaño del mensaje
                        MPI_INT,                // Tipo de dato del mensaje
                        anterior,               // De donde espera recibir
                        TRABAJO,                  // Tag
                        MPI_COMM_WORLD,         // Comunicador global
                        &status                 // Estado de receive
                    );

                    // Almacenar nodos recibidos en la pila;
                    for (int i = 0; i < peticion_trabajo; ++i) {
                        tNodo nuevoNodo;
                        pila->push(nuevoNodo);
                    }



                    break;

                case TESTIGO_DETECCION:
                    cout << "TESTIGO_DETECCION" << endl;
                    break;

                case FIN_DETECTADO:
                    cout << "FIN_DETECTADO" << endl;
                    break;
            }
        }
        cout << "sali del while" << endl << flush;
    }

    if (!(*fin))
    { // el proceso tiene nodos para trabajar
        // Sondear si hay mensajes pendientes de otros procesos;
        cout << "!fin" << endl;
        MPI_Iprobe(
            MPI_ANY_SOURCE,
            MPI_ANY_TAG,
            MPI_COMM_WORLD,
            &flag,
            &status
        );

        while (flag) // atiende peticiones mientras haya mensajes
        { 
            cout << "while(flag)" << endl;
            // Recibir mensaje de peticion de trabajo;
            MPI_Recv(
                &peticion_trabajo,      // Donde recibe
                1,                      // Tamaño del mensaje
                MPI_INT,                // Tipo de dato del mensaje
                anterior,               // De donde espera recibir
                PETICION,               // Tag
                MPI_COMM_WORLD,         // Comunicador global
                &status                 // Estado de receive
            );
            
            if (pila->tamanio() > 2){
                cout << "pila->tamanio > 2" << endl;
                // Enviar nodos al proceso solicitante;
                int numeroNodosEnviados = 1;
                int* nodosEnviados = new int[numeroNodosEnviados];

                for (int i = 0; i < numeroNodosEnviados; ++i) {
                    for (int j = 0; j < 2 * NCIUDADES; ++j) {
                        nodosEnviados[j] = pila->nodos[2* NCIUDADES * i + j];
                    }
                }
                MPI_Send(
                    &nodosEnviados,                         //Información enviada
                    2 * NCIUDADES * numeroNodosEnviados,    //Numero de elementos enviados
                    MPI_INT,                                //Tipo de mensaje
                    siguiente % size,                       //Destinatario del mensaje 
                    TRABAJO,                                // Tag
                    MPI_COMM_WORLD                          //Comunicador por el que se envia
                );
            }
            else{
                cout << "pila->tamanio <= 2" << endl;
                // Pasar peticion de trabajo al proceso(id + 1) % P;
                MPI_Send(
                    &id_Proceso,                //ID de proceso que envia
                    1,                          //Numero de elementos enviados
                    MPI_INT,                    //Tipo de mensaje
                    siguiente % size,           //Destinatario del mensaje (siguiente proceso en anillo)
                    PETICION,                   // Tag
                    MPI_COMM_WORLD              //Comunicador por el que se envia
                );
            }
            
            // Sondear si hay mensajes pendientes de otros procesos;
            MPI_Iprobe(
                MPI_ANY_SOURCE,
                MPI_ANY_TAG,
                MPI_COMM_WORLD,
                &flag,
                &status
            );
        }
    }
}

int main(int argc, char **argv)
{
    int id_Proceso, size;

    //MPI::Init(argc,argv);
    MPI_Init(&argc, &argv);               // Inicializamos la comunicacion de los procesos
    MPI_Comm_size(MPI_COMM_WORLD, &size); // Obtenemos el numero total de hebras
    MPI_Comm_rank(MPI_COMM_WORLD, &id_Proceso); // Obtenemos el valor de nuestro identificador
    
	switch (argc) {
		case 3:		NCIUDADES = atoi(argv[1]);
					break;
		default:	cerr << "La sintaxis es: bbpar <tama�o> <archivo>" << endl;
					exit(1);
					break;
	}
    
    int** tsp0 = reservarMatrizCuadrada(NCIUDADES);
    tNodo	nodo,           // nodo a explorar
			nodo_izq,       // hijo izquierdo
			nodo_dch,       // hijo derecho
			solucion;       // mejor solucion
	bool activo,            // condicion de fin
		nueva_U;            // hay nuevo valor de c.s.
	int  U;                 // valor de c.s.
	int iteraciones = 0;    // numero de iteraciones realizadas
	tPila pila;             // pila de nodos a explorar

    InicNodo (&nodo);   

    //Inicializacion de la c.s.
    U = INFINITO;

    if (id_Proceso == 0){
        // Leer_Problema_Inicial(&nodo);
        LeerMatriz (argv[2], tsp0);    // lee matriz de fichero
    }

    // ... Difusión matriz del problema inicial del proceso 0 al resto
    MPI_Bcast (
        tsp0[0],                       // Dato a compartir
        NCIUDADES * NCIUDADES,      // Número de elementos a enviar y recibir
        MPI_INT,                    // Tipo de dato compartido
        0,                          // Proceso raíz
        MPI_COMM_WORLD              // Comunicador utilizado
    );

    for (int i = 0; i < NCIUDADES; ++i){
        cout << "P(" << id_Proceso << ") || fila " << i << " => ";
        for (int j = 0; j < NCIUDADES; ++j){
            cout << tsp0[i][j]<< " ";
        }
        cout << endl;
    }

    cout << endl;
    bool fin = false;

    if (id_Proceso != 0) {
        Equilibrado_Carga(id_Proceso, size, &pila, &fin);
        if (!fin){
            cout << "prepop" << endl;
            pila.pop(nodo);
            cout << "postpop" << endl;
            //Pop(&pila, &nodo);
        }
    }
    

    // Ciclo del B&B

    while (!fin) {
        Ramifica(&nodo, &nodo_izq, &nodo_dch, tsp0);
        if (Solucion(&nodo_dch))
        {
            // ciclo del Branch&Bound
            if (nodo_dch.ci() < U)
                U = nodo_dch.ci();

        }
        else
        {
            // no es un nodo hoja
            if (nodo_dch.ci() < U){
                // Push(&pila, &nodo_dch);
                pila.push(nodo_dch);
            }
        }
        if (Solucion(&nodo_izq))
        {
            if (nodo_izq.ci() < U){
                // U = ci(nodo_izq); // actualiza c.s.
                U = nodo_izq.ci();
            }
        }
        else
        {
            if (nodo_izq.ci() < U){
                // Push(&pila, &nodo_izq);
                pila.push(nodo_izq);
            }
        }

        cout << "tam pila: " << pila.tamanio() << endl;

        // Se difunde la cota superior
        // int antiguaCS = U;
        // Difusion_Cota_Superior(id_Proceso, size, U);

        // Si hay nueva cota superior
        // if (U != antiguaCS){
        //     // Acotar(&pila, U); // Se cambia la cota de la pila actual
        //     pila.acotar(U);
        // }
        // Se equilibra la carga de la pila
        Equilibrado_Carga(id_Proceso, size, &pila, &fin);

        
        if (!fin){
            // Pop(&pila, &nodo);
            pila.pop(nodo);
        }
    }

    MPI_Finalize();

    if (id_Proceso == 0) {
        liberarMatriz(tsp0);
    }
    return 0;
}