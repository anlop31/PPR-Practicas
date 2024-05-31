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

MPI_Comm COMM_EQUILIBRADO,    
        COMM_DIFUSION,            
        COMM_FIN;    

void Difusion_Cota_Superior(int id_Proceso, int size, int & cs) {
    bool difundir_cs_local = true;
    bool pendiente_retorno_cs = false;
    int cs_local = cs;
    int flag;
    MPI_Status status;
    int anterior = id_Proceso == 0 ? size - 1 : id_Proceso - 1;
    int siguiente = (id_Proceso + 1);

    if (difundir_cs_local && !pendiente_retorno_cs){
        // Enviar valor local de cs al proceso(id + 1) % P;
        MPI_Send(
            &cs_local,          //ID de proceso que envia
            1,                  //Numero de elementos enviados
            MPI_INT,            //Tipo de mensaje
            siguiente,   //Destinatario del mensaje
            ENVIO_CS,           // Tag de peticion de trabajo
            COMM_DIFUSION      //Comunicador por el que se envia
        );
        pendiente_retorno_cs = true;
        difundir_cs_local = false;
    }

    // Sondear si hay mensajes de cota superior pendientes;
    MPI_Iprobe(
        anterior,         //ID del proceso que ha enviado mensaje
        ENVIO_CS,               //Tag con el que se espera recibir el mensaje
        COMM_DIFUSION,         //Comunicador utilizado
        &flag,                  //Flag del probe
        &status                 //MPI_Status del iprobe
    );

    while (flag) // mientras haya mensajes
    {
        // Recibir mensaje con valor de cota superior desde el proceso(id - 1 + P) % P;    
        MPI_Recv(
            &cs_local,                        // Donde recibe
            1,                                // Tamaño del mensaje
            MPI_INT,                          // Tipo de dato del mensaje
            anterior,                  // De donde espera recibir
            ENVIO_CS,                   // Tag
            COMM_DIFUSION,                   // Comunicador global
            &status                           // Estado de receive
        );

        // Actualizar valor local de cota superior;
        if (cs_local < cs){
            cs = cs_local;
        }

        if (status.MPI_SOURCE == id_Proceso && difundir_cs_local) {
            // Enviar valor local de cs al proceso(id + 1) % P;
            MPI_Send(
                &cs,          //ID de proceso que envia
                1,                  //Numero de elementos enviados
                MPI_INT,            //Tipo de mensaje
                siguiente,   //Destinatario del mensaje 
                ENVIO_CS,           // Tag de peticion de trabajo
                COMM_DIFUSION      //Comunicador por el que se envia
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
                &cs,          //ID de proceso que envia
                1,                  //Numero de elementos enviados
                MPI_INT,            //Tipo de mensaje
                siguiente,   //Destinatario del mensaje 
                ENVIO_CS,           //Tag de envío de cota superior
                COMM_DIFUSION      //Comunicador por el que se envia
            );
        }
        // Sondear si hay mensajes de cota superior pendientes;
        MPI_Iprobe(
            anterior,         //ID del proceso que ha enviado mensaje
            ENVIO_CS,               //Tag con el que se espera recibir el mensaje
            COMM_DIFUSION,         //Comunicador utilizado
            &flag,                  //Flag del iprobe
            &status                 //MPI_Status del iprobe
        );
    }
}

// antes: void Dijkstra (int id_Proceso, int size, int &estado, int &color, int &token, bool &token_presente) {
void Dijkstra (int id_Proceso, int size, bool & fin, tPila & pila) {
    // P(id)
    //     ...Esperar evento;
    
    MPI_Status status;
    int flag;
    int siguiente = (id_Proceso + 1) % size;

    int anterior = (id_Proceso == 0) ? size - 1 : id_Proceso - 1;

    MPI_Iprobe(
        MPI_ANY_SOURCE, // De donde espera recibir el mensaje
        MPI_ANY_TAG,    // Espera cualquier tag
        COMM_FIN,       // Comunicador de detección de fin
        &flag,          // Flag
        &status         // Estado del probe
    );
    
    switch (status.MPI_TAG){
        case MENSAJE_TRABAJO:
    //      estado = ACTIVO;
            estado = ACTIVO;
            break;

        case MENSAJE_PETICION:
//            if (hay trabajo para ceder)
            if (pila.tamanio() > 1) //Falta la condicion
            {
//                j = origen(PETICION);
                int origen_peticion = status.MPI_SOURCE; // origen peticion = j
//                Enviar TRABAJO a P(j);
                MPI_Send(
                    &id_Proceso,        //Información enviada
                    1,                  //Numero de elementos enviados
                    MPI_INT,            //Tipo de mensaje
                    origen_peticion,                //Destinatario del mensaje 
                    MENSAJE_TRABAJO,    // Tag
                    COMM_FIN      //Comunicador por el que se envia
                );
//                if (id < j){
//                    mi_color = NEGRO;
//                }
                if (id_Proceso < origen_peticion) {
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
                        fin = true;
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
                        COMM_FIN       //Comunicador por el que se envia
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
                    COMM_FIN       //Comunicador por el que se envia
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
    int anterior = (id_Proceso == 0) ? size - 1 : id_Proceso - 1;
    int siguiente = (id_Proceso + 1) % size;
    int hay_mensajes;
    MPI_Status status;
    int proceso_peticion; // id de quien lo solicita

    cout << "\t\t (equilibrado) tam pila de P(" << id_Proceso << "): " << pila->tamanio() << endl;

    
    if (pila->vacia()){ 
        // el proceso no tiene trabajo: pide a otros procesos
        // Enviar peticion de trabajo al proceso(id + 1) % P;
        MPI_Send(
            &id_Proceso,         //Información enviada
            1,                   //Numero de elementos enviados
            MPI_INT,             //Tipo de mensaje
            siguiente,    //Destinatario del mensaje
            PETICION,            // Tag de peticion de trabajo
            COMM_EQUILIBRADO       //Comunicador por el que se envia
        );

        while (pila->vacia() && !(*fin))
        {
           
            // Esperar mensaje de otro proceso;
            MPI_Probe(
                MPI_ANY_SOURCE, // De donde espera recibir el mensaje
                MPI_ANY_TAG,    // Espera cualquier tag
                COMM_EQUILIBRADO, // Comunicador global
                &status         // Estado del probe
            );

            switch (status.MPI_TAG){
                case PETICION: // Si es petición de trabajo (0)
                    // Recibir mensaje de peticion de trabajo;
                    // cout << "Soy P" << id_Proceso << " y recibo PETICION => source == " << status.MPI_SOURCE << endl << flush;

                    // Recibir mensaje de petición
                    MPI_Recv(
                        &proceso_peticion,       // Donde recibe
                        1,                       // Tamaño del mensaje
                        MPI_INT,                 // Tipo de dato del mensaje
                        anterior,                // De donde espera recibir
                        PETICION,                // Tag
                        COMM_EQUILIBRADO,          // Comunicador global
                        &status                  // Estado de receive
                    );

                    // Reenviar peticion de trabajo al siguiente proceso
                    MPI_Send(
                        &proceso_peticion,         //Información enviada
                        1,                   //Numero de elementos enviados
                        MPI_INT,             //Tipo de mensaje
                        siguiente,    //Destinatario del mensaje 
                        PETICION,            // Tag
                        COMM_EQUILIBRADO       //Comunicador por el que se envia
                    );


                    // if (status.MPI_ERROR == MPI_SUCCESS) {
                    //     cout << "Recv PETICION correcto " << endl << flush;
                    // }
                    // else {
                    //     cout << "Recv PETICION incorrecto" << endl << flush;
                    // }

                    // El mensaje ha dado la vuelta
                    if (proceso_peticion == id_Proceso) // solicitante = id
                    {
                        // cout << "\t\t\t\t\t" << "MPISOURCE == " << id_Proceso << " solicitante: " << proceso_peticion << endl;
                        // cout << "P("<<id_Proceso<<") recibi PETICION de " << proceso_peticion << ", posible fin" << endl;
                        // Iniciar deteccion de posible situacion de fin;
                        
                        //Cuando esté correcto el equilibrado de carga probar
                        //con la deteccion de fin
                        // Dijkstra(id_Proceso, size, *fin, *pila);
                    }

                    break;
                case TRABAJO: // Si recibe nodos (1)
                    // resultado de una peticion de trabajo
                    // Recibir nodos del proceso donante;
                    cout << "-->P("<<id_Proceso<<") recibo nodos..." << endl;

                    estado = ACTIVO;
                    int numeroNodos;
                    MPI_Get_count(&status, MPI_INT, &numeroNodos);

                    MPI_Recv(
                        &pila->nodos[0],      // Donde recibe
                        numeroNodos,         // Tamaño del mensaje
                        MPI_INT,                // Tipo de dato del mensaje
                        status.MPI_SOURCE,               // De donde espera recibir
                        TRABAJO,                // Tag
                        COMM_EQUILIBRADO,         // Comunicador global
                        &status                 // Estado de receive
                    );

                    // Almacenar nodos recibidos en la pila;    
                    pila->tope = numeroNodos;

                    cout << "-->P("<<id_Proceso<<") numero nodos recibidos: " << numeroNodos << "del proceso: " << status.MPI_SOURCE << endl;

                    break;

                // case TESTIGO_DETECCION:
                //     cout << "TESTIGO_DETECCION" << endl;
                //     break;

                // case FIN_DETECTADO:
                //     cout << "FIN_DETECTADO" << endl;
                //     break;
            }
        }
        cout<<"P("<<id_Proceso<<")sali del while porque ya no tengo pila vacia (tam: "<<pila->tamanio()<<")" << endl << flush;
    }

    if (!(*fin))
    { // el proceso tiene nodos para trabajar
        // Sondear si hay mensajes pendientes de otros procesos;
        cout << "--P("<<id_Proceso<<") entro en proceso de sondeo" << endl;
        
        MPI_Iprobe(
            MPI_ANY_SOURCE,
            MPI_ANY_TAG,
            COMM_EQUILIBRADO,
            &hay_mensajes,
            &status
        );

//////////////////// 
        // int cantidadMensajes;
        // MPI_Probe(
        //     MPI_ANY_SOURCE,
        //     MPI_ANY_TAG,
        //     COMM_EQUILIBRADO,
        //     // &hay_mensajes,
        //     &status
        // );
        // MPI_Get_count(&status, MPI_INT, &cantidadMensajes);
////////////////////

        cout << "----despues de iprobe con hay_mensajes: " << hay_mensajes << endl;

        while (hay_mensajes > 0) // atiende peticiones mientras haya mensajes
        { 
            // cout << "while(hay_mensajes)" << endl;
            // Recibir mensaje de peticion de trabajo;
            MPI_Recv(
                &proceso_peticion,      // Donde recibe
                1,                      // Tamaño del mensaje
                MPI_INT,                // Tipo de dato del mensaje
                anterior,               // De donde espera recibir
                PETICION,               // Tag
                COMM_EQUILIBRADO,         // Comunicador global
                &status                 // Estado de receive
            );
            
            if (pila->tamanio() > 1){
                // Enviar nodos al proceso solicitante;

                // Dividir la pila por la mitad
                tPila pila2;
                pila->divide(pila2);

                // Enviar nodos al proceso solicitante
                MPI_Send(
                    &pila2.nodos[0],                        // Información enviada
                    pila2.tope,                             // Numero de elementos enviados
                    MPI_INT,                                // Tipo de mensaje
                    proceso_peticion,                       // Destinatario del mensaje 
                    TRABAJO,                                // Tag
                    COMM_EQUILIBRADO                          // Comunicador por el que se envia
                );
            }
            else{
                // Pasar peticion de trabajo al proceso(id + 1) % P;
                MPI_Send(
                    &proceso_peticion,                //ID de proceso que envia
                    1,                          //Numero de elementos enviados
                    MPI_INT,                    //Tipo de mensaje
                    siguiente,           //Destinatario del mensaje (siguiente proceso en anillo)
                    PETICION,                   // Tag
                    COMM_EQUILIBRADO              //Comunicador por el que se envia
                );
            }
            
            // Sondear si hay mensajes pendientes de otros procesos;
            MPI_Iprobe(
                MPI_ANY_SOURCE,
                MPI_ANY_TAG,
                COMM_EQUILIBRADO,
                &hay_mensajes,
                &status
            );

//////////////////////
            // MPI_Probe(
            //     MPI_ANY_SOURCE,
            //     MPI_ANY_TAG,
            //     MPI_COMM_WORLD,
            //     // &hay_mensajes,
            //     &status
            // );
            // MPI_Get_count(&status, MPI_INT, &cantidadMensajes);     
            // cout << "probe de despues con " << cantidadMensajes << " mensajes" << endl;       
//////////////////////
        } // fin while hay mensajes
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

    // Creación de comunicadores para el equilibrado, al difusión de cota y deteccion de fin
    // extern MPI_Comm COMM_EQUILIBRADO,    
    //             COMM_DIFUSION,            
    //             COMM_FIN;                 


    int colorEquilibrado = 0; // columnas

    MPI_Comm_split (MPI_COMM_WORLD,     // a partir del comunicador global
                    colorEquilibrado,       // los del mismo color entraran en el mismo comunicador
                    id_Proceso,         // indica el orden de asignacion de rango dentro del nuevo comm
                    &COMM_EQUILIBRADO);    // referencia al nuevo comunicador

    int colorDifusion = 1; // columnas

    MPI_Comm_split (MPI_COMM_WORLD,     // a partir del comunicador global
                    colorDifusion,       // los del mismo color entraran en el mismo comunicador
                    id_Proceso,         // indica el orden de asignacion de rango dentro del nuevo comm
                    &COMM_DIFUSION);    // referencia al nuevo comunicador

    int colorFin = 2; // columnas

    MPI_Comm_split (MPI_COMM_WORLD,     // a partir del comunicador global
                    colorFin,       // los del mismo color entraran en el mismo comunicador
                    id_Proceso,         // indica el orden de asignacion de rango dentro del nuevo comm
                    &COMM_FIN);    // referencia al nuevo comunicador


    
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

    // DEBUG: ver que todos los procesos tienen la matriz bien
    // for (int i = 0; i < NCIUDADES; ++i){
    //     cout << "P(" << id_Proceso << ") || fila " << i << " => ";
    //     for (int j = 0; j < NCIUDADES; ++j){
    //         cout << tsp0[i][j]<< " ";
    //     }
    //     cout << endl;
    // }

    cout << endl;
    bool fin = false;

    if (id_Proceso != 0) {
        Equilibrado_Carga(id_Proceso, size, &pila, &fin);
        if (!fin){
            // cout << "pop" << endl;
            pila.pop(nodo);
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

        // DEBUG
        // cout << "(main) tam pila (P:"<<id_Proceso<<"): " << pila.tamanio() << endl;

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
