#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <mpi.h>
#include "libbb.h"

using namespace std;

unsigned int NCIUDADES;
int rank, size;
const int PETIC = 0, 
          NODOS = 1;

const int MENSAJE_TRABAJO = 0,
          MENSAJE_PETICION = 1,
          MENSAJE_TOKEN = 2,
          TRABAJO_AGOTADO = 3;

const int ENVIO_CS = 0;

void Difusion_Cota_Superior() {
    bool difundir_cs_local = true;
    bool pendiente_retorno_cs = false;
    int cs_local = 0
    if (difundir_cs_local && !pendiente_retorno_cs)
    {
        // Enviar valor local de cs al proceso(id + 1) % P;
        MPI_Send(
            cs_local,           //ID de proceso que envia
            1,                  //Numero de elementos enviados
            MPI_INT,            //Tipo de mensaje
            (rank+1) % size,    //Destinatario del mensaje (siguiente proceso en anillo)
            ENVIO_CS,           // Tag de peticion de trabajo
            MPI_COMM_WORLD      //Comunicador por el que se envia
        );
        pendiente_retorno_cs = true;
        difundir_cs_local = false;
    }
    // Sondear si hay mensajes de cota superior pendientes;
    MPI_Status status_probe;

    MPI_Probe(
        MPI_ANY_SOURCE, // De donde espera recibir el mensaje
        ENVIO_CS,       // Espera cualquier tag
        MPI_COMM_WORLD, // Comunicador global
        &status_probe   // Estado del probe
    );
    while (status_probe.size != 0)
    {
        // Recibir mensaje con valor de cota superior desde el proceso(id - 1 + P) % P;
        
        MPI_Status status_recv;
        MPI_Recv(
            cs_local,                   // Donde recibe
            1,                          // Tamaño del mensaje
            MPI_INT,                    // Tipo de dato del mensaje
            (rank - 1 + size) % size,   // De donde espera recibir
            status_probe.MPI_TAG,   // Tag
            ENVIO_CS,         // Comunicador global
            &status_recv            // Estado de receive
        );

        // Actualizar valor local de cota superior;
        if (status_recv.MPI_SOURCE == rank && difundir_cs_local) {
            // Enviar valor local de cs al proceso(id + 1) % P;
            MPI_Send(
                cs_local,           //ID de proceso que envia
                1,                  //Numero de elementos enviados
                MPI_INT,            //Tipo de mensaje
                (rank+1) % size,    //Destinatario del mensaje (siguiente proceso en anillo)
                ENVIO_CS,           // Tag de peticion de trabajo
                MPI_COMM_WORLD      //Comunicador por el que se envia
            );
            pendiente_retorno_cs = true;
            difundir_cs_local = false;
        }
        else if (status_recv.MPI_SOURCE == rank && !difundir_cs_local) {
            pendiente_retorno_cs = false;
        }
        else{ // origen mensaje == otro proceso
            // Reenviar mensaje al proceso(id + 1) % P;
            MPI_Send(
                cs_local,           //ID de proceso que envia
                1,                  //Numero de elementos enviados
                MPI_INT,            //Tipo de mensaje
                (rank+1) % size,    //Destinatario del mensaje (siguiente proceso en anillo)
                ENVIO_CS,           // Tag de peticion de trabajo
                MPI_COMM_WORLD      //Comunicador por el que se envia
            );
        }
        // Sondear si hay mensajes de cota superior pendientes;
        MPI_Probe(
            MPI_ANY_SOURCE, // De donde espera recibir el mensaje
            ENVIO_CS,       // Espera cualquier tag
            MPI_COMM_WORLD, // Comunicador global
            &status_probe   // Estado del probe
        );
    }
}

void Dijkstra () {
    // P(id)
    //     ...Esperar evento;
    // switch (tipo_evento){
    //     case MENSAJE_TRABAJO:
    //         estado = ACTIVO;
    //     case MENSAJE_PETICION:
    //         if (hay trabajo para ceder)
    //         {
    //             j = origen(PETICION);
    //             Enviar TRABAJO a P(j);
    //             if (id < j)
    //                 mi_color = NEGRO;
    //         }
    //     case MENSAJE_TOKEN:
    //         token_presente = TRUE;
    //         if (estado == PASIVO)
    //         {
    //             if (id == 0 && mi_color == BLANCO && color(TOKEN) == BLANCO)
    //                 TERMINACION DETECTADA;
    //             else
    //             {
    //                 if (id == 0)
    //                     color(TOKEN) = BLANCO;
    //                 else if (mi_color == NEGRO)
    //                     color(TOKEN) = NEGRO;
    //                 Enviar TOKEN a P(id - 1);
    //                 mi_color = BLANCO;
    //                 token_presente = FALSE;
    //             }
    //         }
    //     case TRABAJO_AGOTADO:
    //         estado = PASIVO;
    //         if (token_presente)
    //             if (id == 0)
    //                 color(TOKEN) = BLANCO;
    //             else if (mi_color == NEGRO)
    //                 color(TOKEN) = NEGRO;
    //         Enviar TOKEN a P(id - 1);
    //         mi_color = BLANCO;
    //         token_presente = FALSE;
    //         ...
    // }
}

void Equilibrado_Carga(tPila * pila, bool *fin)
{
    if (pila.vacia())
    { 
        // el proceso no tiene trabajo: pide a otros procesos
        // Enviar peticion de trabajo al proceso(id + 1) % P;
        MPI_Send(
            rank,               //ID de proceso que envia
            1,                  //Numero de elementos enviados
            MPI_INT,            //Tipo de mensaje
            (rank+1) % size,    //Destinatario del mensaje (siguiente proceso en anillo)
            PETIC,              // Tag de peticion de trabajo
            MPI_COMM_WORLD      //Comunicador por el que se envia
        );

        // tag -> 0: peticion de trabajo
        int source;
        if (rank == 0){
            source = size - 1;
        }
        else {
            source = rank - 1;
        }
        while (pila.vacia() && !fin)
        {
            MPI_Status status_probe;

            // Esperar mensaje de otro proceso;
            MPI_Probe(
                source,         // De donde espera recibir el mensaje
                MPI_ANY_TAG,    // Espera cualquier tag
                MPI_COMM_WORLD, // Comunicador global
                &status_probe   // Estado del probe
            );

            switch (status_probe.MPI_TAG){
                case PETIC: // Si es petición de trabajo (0)
                    // peticion de trabajo
                    // Recibir mensaje de peticion de trabajo;
                    int peticion_trabajo; // id de quien lo solicita
                    MPI_Status status_recv;
                    MPI_Recv(
                        peticion_trabajo,       // Donde recibe
                        status_probe.size,      // Tamaño del mensaje
                        MPI_INT,                // Tipo de dato del mensaje
                        source,                 // De donde espera recibir
                        status_probe.MPI_TAG,   // Tag
                        MPI_COMM_WORLD,         // Comunicador global
                        &status_recv            // Estado de receive
                    );

                    if (status_recv.MPI_SOURCE == rank) // solicitante = id
                    {
                        // peticion devuelta
                        // Reenviar peticion de trabajo al proceso(id + 1) % P;
                        MPI_Send(
                            rank,                   //ID de proceso que envia
                            status_recv.size,       //Numero de elementos enviados
                            MPI_Status,             //Tipo de mensaje
                            (rank+1) % size,        //Destinatario del mensaje (siguiente proceso en anillo)
                            status_recv.MPI_TAG,    // Tag
                            MPI_COMM_WORLD          //Comunicador por el que se envia
                        );
                        // Iniciar deteccion de posible situacion de fin;
                    }
                    else
                        // peticion de otro proceso: la retransmite al siguiente
                        // Pasar peticion de trabajo al proceso(id + 1) % P;
                        MPI_Send(
                            rank,                   //ID de proceso que envia
                            status_recv.size,       //Numero de elementos enviados
                            MPI_Status,             //Tipo de mensaje
                            (rank+1) % size,        //Destinatario del mensaje (siguiente proceso en anillo)
                            status_recv.MPI_TAG,    // Tag
                            MPI_COMM_WORLD          //Comunicador por el que se envia
                        );
                        break;
                case NODOS: // Si recibe nodos (1)
                    // resultado de una peticion de trabajo
                    // Recibir nodos del proceso donante;
                    int peticion_trabajo; // id de quien lo solicita
                    MPI_Status status_recv;
                    MPI_Recv(
                        peticion_trabajo,       // Donde recibe
                        status_probe.size,      // Tamaño del mensaje
                        MPI_INT,                // Tipo de dato del mensaje
                        source,                 // De donde espera recibir
                        status_probe.MPI_TAG,   // Tag
                        MPI_COMM_WORLD,         // Comunicador global
                        &status_recv            // Estado de receive
                    );
                    // Almacenar nodos recibidos en la pila;
                    break;
            }
        }
    }

    if (!fin)
    { // el proceso tiene nodos para trabajar
        // Sondear si hay mensajes pendientes de otros procesos;
        int * flag;
        MPI_Status status_iprobe;
        MPI_Iprobe(
            MPI_ANY_SOURCE,
            MPI_ANY_TAG,
            MPI_COMM_WORLD,
            &flag,
            &status_iprobe
        );

        while (hay mensajes)
        { 
            // atiende peticiones mientras haya mensajes
            // Recibir mensaje de peticion de trabajo;
            int peticion_trabajo; // id de quien lo solicita
            MPI_Status status_recv;
            MPI_Recv(
                peticion_trabajo,       // Donde recibe
                status_probe.size,      // Tamaño del mensaje
                MPI_INT,                // Tipo de dato del mensaje
                source,                 // De donde espera recibir
                status_probe.MPI_TAG,   // Tag
                MPI_COMM_WORLD,         // Comunicador global
                &status_recv            // Estado de receive
            );
            
            if (pila->tamanio > 2){
                // Enviar nodos al proceso solicitante;
                
            }
            else{
                // Pasar peticion de trabajo al proceso(id + 1) % P;
            }
            
            // Sondear si hay mensajes pendientes de otros procesos;
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
		case 4:		NCIUDADES = atoi(argv[1]);
					break;
		default:	cerr << "La sintaxis es: bbpar <tama�o> <archivo> <numeroProcesos>" << endl;
					exit(1);
					break;
	}
    
    int* tsp0 = reservarMatrizCuadrada1D(NCIUDADES);
    tNodo	nodo,           // nodo a explorar
			lnodo,          // hijo izquierdo
			rnodo,          // hijo derecho
			solucion;       // mejor solucion
	bool activo,            // condicion de fin
		nueva_U;            // hay nuevo valor de c.s.
	int  U;                 // valor de c.s.
	int iteraciones = 0;    // numero de iteraciones realizadas
	tPila pila;             // pila de nodos a explorar

    //Inicializacion de la c.s.
    U = INFINITO;

    if (id_Proceso == 0){
        // Leer_Problema_Inicial(&nodo);
        LeerMatriz1D (argv[2], tsp0);    // lee matriz de fichero
    }

    // ... Difusión matriz del problema inicial del proceso 0 al resto
    MPI_Bcast (
        tsp0,                       // Dato a compartir
        NCIUDADES * NCIUDADES,      // Número de elementos a enviar y recibir
        MPI_INT,                    // Tipo de dato compartido
        0,                          // Proceso raíz
        MPI_COMM_WORLD              // Comunicador utilizado
    );

    for (int i = 0; i < NCIUDADES; ++i){
        cout << "P(" << id_Proceso << ") || fila " << i << " => ";
        for (int j = 0; j < NCIUDADES; ++j){
            cout << tsp0[i * NCIUDADES + j]<< " ";
        }
        cout << endl;
    }

    cout << endl;
    bool fin = false;
    if (id_Proceso != 0) {
        Equilibrado_Carga(&pila, &fin);
        if (!fin)
            pila.pop(nodo);
            //Pop(&pila, &nodo);
    }

    // // Ciclo del B&B
    // while (!fin) {
    //     Ramifica(&nodo, &nodo_izq, &nodo_dch);
    //     if (Solucion(&nodo_dch))
    //     {
    //         // ciclo del Branch&Bound
    //         if (ci(nodo_dch) < U)
    //             U = ci(nodo_dch);
    //     }
    //     else
    //     {
    //         // no es un nodo hoja
    //         if (ci(nodo_dch) < U)
    //             Push(&pila, &nodo_dch);
    //     }
    //     if (Solucion(&nodo_izq))
    //     {
    //         if (ci(nodo_izq) < U)
    //             U = ci(nodo_izq); // actualiza c.s.
    //     }
    //     else
    //     {
    //         if (ci(nodo_izq) < U)
    //             Push(&pila, &nodo_izq);
    //     }

    //     // Se difunde la cota superior
    //     Difusion_Cota_Superior(&U);

    //     // Si hay nueva cota superior
    //     if (hay_nueva_cota_superior)
    //         Acotar(&pila, U); // Se cambia la cota de la pila actual

    //     // Se equilibra la carga de la pila
    //     Equilibrado_Carga(&pila, &fin);

        
    //     if (!fin)
    //         Pop(&pila, &nodo);
    // }

    MPI_Finalize();

    if (id_Proceso == 0) {
        liberarMatriz1D(tsp0);
    }
    return 0;
}