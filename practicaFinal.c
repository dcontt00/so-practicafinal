#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>


pthread_mutex_t mutexFichero, mutexColaPacientes;
pthread_cond_t varEstadistico,varPacientes;
int pacientes;
const int MAXPACIENTES=15;
struct paciente
{
    int id;
    int atendido; // 0 si no ha sido atendido 1 en caso contrario
    
    /**
     * Junior(0-16 años): 0
     * Medios(16-60 años): 1
     * Senior(60+ años): 2
     */ 
    int tipo;
    int serologia; // 0 si no participa 1 en caso contrario
};
struct paciente listaPacientes[MAXPACIENTES];

struct enfermero
{
    int id;
    char grupoVacunacion[20]; // el enfermero trata Junior, Medios o Senior
    int pacientesAtendidos;
};
struct enfermero enfermero1,enfermero2,enfermero3;

pthread_t medico, estadistico;

char logFileName;
FILE *logFile;
int main(int argc, char argv[]){

//1. signal o sigaction SIGUSR1, paciente junior.
	signal(SIGUSR1, /**nuevoPaciente**/);

//2. signal o sigaction SIGUSR2, paciente medio.
	signal(SIGUSR2, /**nuevoPaciente**/);

//3. signal o sigaction SIGPIPE, paciente senior.
	signal(SIGPIPE, /**nuevoPaciente**/);

//4. signal o sigaction SIGINT, terminar
//5. Inicializar recursos (¡Ojo!, Inicializar!=Declarar).
    //a. Semáforos.
    if (pthread_mutex_init(&mutexFichero, NULL)!=0){
        exit(-1);
    }
    if (pthread_mutex_init(&mutexColaPacientes, NULL)!=0){
        exit(-1);
    }  
    //b. Contador de pacientes.
    pacientes=0;

    //c. Lista de pacientes id 0, atendido 0, tipo 0, serología 0.
    for (size_t i = 0; i < MAXPACIENTES; i++)
    {
        listaPacientes[i].atendido=0;
        listaPacientes[i].id=0;
        listaPacientes[i].serologia=0;
        listaPacientes[i].tipo=0;
    }

    //d. Lista de enfermer@s (si se incluye).
    //e. Fichero de Log
    //f. Variables condición
    if (pthread_cond_init(&varEstadistico, NULL)!=0){
        exit(-1); 
    } 
    if (pthread_cond_init(&varPacientes, NULL)!=0){
        exit(-1); 
    } 
//6. Crear 3 hilos enfermer@s.
    pthread_create (&medico, NULL, hiloMedico, NULL);
    pthread_create (&medico, NULL, hiloMedico, NULL);
    pthread_create (&medico, NULL, hiloMedico, NULL);

//7. Crear el hilo médico.
    pthread_create (&medico, NULL, hiloMedico, NULL);
//8. Crear el hilo estadístico.
    pthread_create (&estadistico, NULL, hiloEstadistico, NULL);
//9. Esperar por señales de forma infinita.
}

/*
 * Hilo que representa al médico
 */
void *hiloMedico(void *arg){
	pthread_exit(NULL);
}

/*
 * Hilo que representa al Estadístico
 */
void *hiloEstadistico(void *arg){
	pthread_exit(NULL);
}

void writeLogMessage(char *id, char *msg) {
    // Calculamos la hora actual
    time_t now = time(0);
    struct tm *tlocal = localtime(&now);
    char stnow[25];
    strftime(stnow, 25, "%d/%m/%y %H:%M:%S", tlocal);
    // Escribimos en el log
    logFile = fopen(logFileName, "a");
    fprintf(logFile, "[%s] %s: %s\n", stnow, id, msg);
    fclose(logFile);
}
