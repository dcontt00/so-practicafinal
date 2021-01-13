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
struct Paciente
{
    int id;// Identificacion del paciente
    int atendido; // 0 si no ha sido atendido 1 en caso contrario
    
    /**
     * Junior(0-16 años): 0
     * Medios(16-60 años): 1
     * Senior(60+ años): 2
     */ 
    int tipo;
    int serologia; // 0 si no participa 1 en caso contrario
};
struct Paciente listaPacientes[MAXPACIENTES];

struct Enfermero
{
    int id;
    /**
     * Junior(0-16 años): 0
     * Medios(16-60 años): 1
     * Senior(60+ años): 2
     */ 
    int grupoVacunacion; // el enfermero trata Junior, Medios o Senior
    int atendiendo //0 si esta libre, 1 si esta atendiendo a un paciente
    int pacientesAtendidos;
};
struct Enfermero enfermero1,enfermero2,enfermero3;

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
    pthread_create (&enfermero1, NULL, hiloMedico, NULL);
    pthread_create (&enfermero2, NULL, hiloMedico, NULL);
    pthread_create (&enfermero3, NULL, hiloMedico, NULL);

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
	while(true){
		pthread_mutex_lock(mutexColaPacientes);
		//Variable que guarda la posición del paciente que se busca
		int posPaciente = -1;
		/*
		 * Buscamos el paciente con reaccion que mas tiempo lleve esperando
		 * Si no hay posPaciente seguira siendo -1 y si hay pasa a ser la 
		 * posicion del paciente.
		 */
		for(int i = 0; i < MAXPACIENTES && posPaciente == 0; i++){

		}
		pthread_mutex_unlock(mutexColaPacientes);
	}
}

/*
 * Hilo que representa al Enfermrer@
 */
void *hiloEnfermero(void *arg) {
    while(1) {
        switch(grupoVacunacion) { //Sabiendo el grupo al que  vacuna realizara buscara en un sitio u otro
            case 0: 
                printf("Soy el enfermer@_%d", grupoVacunacion + 1); //Asignamos al enfermero su identificador secuencial

                for(int i = 0; i < MAXPACIENTES; i++) {
                    if(enfermero1.atendiendo == 0 &&listaPacientes[i].tipo == 0 && listaPacientes[i].atendido == 0) {  //Comprobamos si hay del mismo tipo, si ha sido atendido y si ese enfermero esta atendiendo
                        enfermero1.atendiendo = 1;
                        enfermero1.pacientesAtendidos++;

                        listaPacientes[i].atendido = 1;//Marcamos el paciente como atendido
                    }
                }

                //No hay pacientes de tipo1, buscamos de otros tipos
                for(int i = 0; i < MAXPACIENTES; i++) {
                    if(enfermero1.atendiendo == 0 && listaPacientes[i].atendido == 0) {  
                        enfermero1.atendiendo = 1;
                        enfermero1.pacientesAtendidos++;

                        listaPacientes.pacientesAtendidos = 1;//Marcamos el paciente como atendido
                    }
                }

                if(enfermero1.pacientesAtendidos == 5) { //Si es 5 entonces podra descansar
                    enfermero1.atendiendo = 0;//
                    enfermero1.pacientesAtendidos = 0; //Resetemaos el contador de pacientes para que pueda volver a empezar
                    sleep(5); //Descansa sus 5 segundos 
                    //Aqui creo que habra que indicar a otro enfermero o al medico que debe vacunar
                }
            case 1:
                printf("Soy el enfermer@_%d", grupoVacunacion + 1); //Asignamos al enfermero su identificador secuencial

                for(int i = 0; i < MAXPACIENTES; i++) {
                    if(enfermero2.atendiendo == 0 &&listaPacientes[i].tipo == 0 && listaPacientes[i].atendido == 0) {  //Comprobamos si hay del mismo tipo, si ha sido atendido y si ese enfermero esta atendiendo
                        enfermero2.atendiendo = 1;
                        enfermero2.pacientesAtendidos++;

                        listaPacientes[i].atendido = 1;//Marcamos el paciente como atendido
                    }
                }

                //No hay pacientes de tipo1, buscamos de otros tipos
                for(int i = 0; i < MAXPACIENTES; i++) {
                    if(enfermero2.atendiendo == 0 && listaPacientes[i].atendido == 0) {  
                        enfermero2.atendiendo = 1;
                        enfermero2.pacientesAtendidos++;

                        listaPacientes.pacientesAtendidos = 1;//Marcamos el paciente como atendido
                    }
                }

                if(enfermero2.pacientesAtendidos == 5) { //Si es 5 entonces podra descansar
                    enfermero2.atendiendo = 0;//
                    enfermero2.pacientesAtendidos = 0; //Resetemaos el contador de pacientes para que pueda volver a empezar
                    sleep(5); //Descansa sus 5 segundos 
                    //Aqui creo que habra que indicar a otro enfermero o al medico que debe vacunar
                }
        }
    }
}

/*
 * Hilo que representa al Estadístico
 */
void *hiloEstadistico(void *arg){
	
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
