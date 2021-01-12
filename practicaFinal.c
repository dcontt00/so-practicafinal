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
struct paciente
{
    int id;
    int atendido; // 0 si no ha sido atendido 1 en caso contrario
    
    /**
     * Junior(0-16 años) SIGUSR1
     * Medios(16-60 años) SIGUSR2
     * Senior(60+ años) SIGPIPE
     */ 
    char tipo [20];
    int serologia; // 0 si no participa 1 en caso contrario
};
struct paciente listaPacientes[15];

struct enfermero
{
    int id;
    char grupoVacunacion[20]; // el enfermero trata Junior, Medios o Senior
    int pacientesAtendidos;
};
struct enfermero listaEnfermeros[3];

pthread_t medico, estadistico;

char logFileName;
FILE *logFile;
int main(int argc, char argv[]){

//1. signal o sigaction SIGUSR1, paciente junior.
//2. signal o sigaction SIGUSR2, paciente medio.
//3. signal o sigaction SIGPIPE, paciente senior.
//4. signal o sigaction SIGINT, terminar
//5. Inicializar recursos (¡Ojo!, Inicializar!=Declarar).
//  a. Semáforos.
    if (pthread_mutex_init(&mutexFichero, NULL)!=0){
        exit(-1);
    }
    if (pthread_mutex_init(&mutexColaPacientes, NULL)!=0){
        exit(-1);
    }  
//  b. Contador de pacientes.
    pacientes=0;
//  c. Lista de pacientes id 0, atendido 0, tipo 0, serología 0.
//  d. Lista de enfermer@s (si se incluye).
//  e. Fichero de Log
//  f. Variables condición
    if (pthread_cond_init(&varEstadistico, NULL)!=0){
        exit(-1); 
    } 
    if (pthread_cond_init(&varPacientes, NULL)!=0){
        exit(-1); 
    } 
//6. Crear 3 hilos enfermer@s.
//7. Crear el hilo médico.
//8. Crear el hilo estadístico.
//9. Esperar por señales de forma infinita.
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