#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <mpi.h>
#include "libbb.h"

using namespace std;

unsigned int NCIUDADES;
int rank, size;

Difusion_Cota_Superior() {
    if (difundir_cs_local && !pendiente_retorno_cs)
    {
        Enviar valor local de cs al proceso(id + 1) % P;
        pendiente_retorno_cs = TRUE;
        difundir_cs_local = FALSE;
    }
    Sondear si hay mensajes de cota superior pendientes;
    while (hay mensajes)
    {
        Recibir mensaje con valor de cota superior desde el proceso(id - 1 + P) % P;
        Actualizar valor local de cota superior;
        if (origen mensaje == id && difundir_cs_local)
        {
            Enviar valor local de cs al proceso(id + 1) % P;
            pendiente_retorno_cs = TRUE;
            difundir_cs_local = FALSE;
        }
        else if (origen mensaje == id && !difundir_cs_local)
            pendiente_retorno_cs = FALSE;
        else
            // origen mensaje == otro proceso
            Reenviar mensaje al proceso(id + 1) % P;
        Sondear si hay mensajes de cota superior pendientes;
    }
}

Dijkstra () {
    P(id)
        ...Esperar evento;
    switch (tipo_evento)
    {
    case MENSAJE_TRABAJO:
        estado = ACTIVO;
    9 case MENSAJE_PETICION:
        if (hay trabajo para ceder)
        {
            j = origen(PETICION);
            Enviar TRABAJO a P(j);
            if (id < j)
                mi_color = NEGRO;
        }
    case MENSAJE_TOKEN:
        token_presente = TRUE;
        if (estado == PASIVO)
        {
            if (id == 0 && mi_color == BLANCO && color(TOKEN) == BLANCO)
                TERMINACION DETECTADA;
            else
            {
                if (id == 0)
                    color(TOKEN) = BLANCO;
                else if (mi_color == NEGRO)
                    color(TOKEN) = NEGRO;
                Enviar TOKEN a P(id - 1);
                mi_color = BLANCO;
                token_presente = FALSE;
            }
        }
    case TRABAJO_AGOTADO:
        estado = PASIVO;
        if (token_presente)
            if (id == 0)
                color(TOKEN) = BLANCO;
            else if (mi_color == NEGRO)
                color(TOKEN) = NEGRO;
        Enviar TOKEN a P(id - 1);
        mi_color = BLANCO;
        token_presente = FALSE;
        ...
    }
}

Equilibrado_Carga(tPila * pila, bool *fin)
{

    if (Vacia(pila))
    { // el proceso no tiene trabajo: pide a otros procesos
        Enviar peticion de trabajo al proceso(id + 1) % P;
        while (Vacia(pila) && !fin)
        {
            Esperar mensaje de otro proceso;
            switch (tipo de mensaje)
            {
            case PETIC:
                // peticion de trabajo
                Recibir mensaje de peticion de trabajo;
                if (solicitante == id)
                {
                    // peticion devuelta
                    Reenviar peticion de trabajo al proceso(id + 1) % P;
                    Iniciar deteccion de posible situacion de fin;
                }
                else
                    // peticion de otro proceso: la retransmite al siguiente
                    Pasar peticion de trabajo al proceso(id + 1) % P;
                    break;
            case NODOS:
                // resultado de una peticion de trabajo
                Recibir nodos del proceso donante;
                Almacenar nodos recibidos en la pila;
                break;
            }
        }
    }

    if (!fin)
    { // el proceso tiene nodos para trabajar
        Sondear si hay mensajes pendientes de otros procesos;
        while (hay mensajes)
        { // atiende peticiones mientras haya mensajes
            Recibir mensaje de peticion de trabajo;
            if (hay suficientes nodos en la pila para ceder)
                Enviar nodos al proceso solicitante;
                else Pasar peticion de trabajo al proceso(id + 1) % P;
            Sondear si hay mensajes pendientes de otros procesos;
        }
    }
}

int main(int argc, char **argv)
{
    int id_Proceso, size;

    MPI::Init(argc,argv);

	switch (argc) {
		case 4:		NCIUDADES = atoi(argv[1]);
					break;
		default:	cerr << "La sintaxis es: bbseq <tama�o> <archivo> <numeroProcesos>" << endl;
					exit(1);
					break;
	}


    U = INFINITO;
    
    int** tsp0 = reservarMatrizCuadrada(NCIUDADES);
    tNodo	nodo,         // nodo a explorar
			lnodo,        // hijo izquierdo
			rnodo,        // hijo derecho
			solucion;     // mejor solucion
	bool activo,        // condicion de fin
		nueva_U;       // hay nuevo valor de c.s.
	int  U;             // valor de c.s.
	int iteraciones = 0;
	tPila pila;         // pila de nodos a explorar

    // inicializa cota superior
    if (id == 0){
        // Leer_Problema_Inicial(&nodo);
        LeerMatriz (argv[2], tsp0);    // lee matriz de fichero

    }
    // ... Difusión matriz del problema inicial del proceso 0 al resto
    
    if (id != 0) {
        Equilibrar_Carga(&pila, &fin);
        if (!fin)
            Pop(&pila, &nodo);
    }

    // Ciclo del B&B
    while (!fin) {
        Ramifica(&nodo, &nodo_izq, &nodo_dch);
        if (Solucion(&nodo_dch))
        {
            // ciclo del Branch&Bound
            if (ci(nodo_dch) < U)
                U = ci(nodo_dch);
        }
        else
        {
            // no es un nodo hoja
            if (ci(nodo_dch) < U)
                Push(&pila, &nodo_dch);
        }
        if (Solucion(&nodo_izq))
        {
            if (ci(nodo_izq) < U)
                U = ci(nodo_izq); // actualiza c.s.
        }
        else
        {
            if (ci(nodo_izq) < U)
                Push(&pila, &nodo_izq);
        }

        Difusion_Cota_Superior(&U);
        if (hay_nueva_cota_superior)
            Acotar(&pila, U);
        Equilibrado_Carga(&pila, &fin);
        if (!fin)
            Pop(&pila, &nodo);
    }
}