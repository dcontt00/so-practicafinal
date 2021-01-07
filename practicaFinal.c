#include <time.h>
#include <stdio.h>
#include <stdlib.h>

char logFileName;
FILE *logFile;

//Semáforos y variables condición
//o Fichero, colaPacientes
//o Condiciones para el estadístico y los pacientes en el estudio (variable
//paciente en estudio)
int pacientes=0;
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

//• Médico
//• Estadístico



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