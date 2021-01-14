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


/*
 *Declaracion de funciones a utilizar
 */
int calculaRandom(int n1, int n2);
void writeLogMessage(char *id, char *msg);

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
	//Se ejecuta indefinidamente hasta que se recibe el la señal
	while(1){
		//Variable que guarda la posición del paciente que se busca
		int posPaciente = -1;

		while(posPaciente == -1){
			if(pacientes > 0){
				//Como accedemos a la lista bloqueamos el mutex
				pthread_mutex_lock(&mutexColaPacientes);
				/*
				 * Buscamos el paciente con reaccion que mas tiempo lleve esperando
				 * Si no hay posPaciente seguira siendo -1 y si hay pasa a ser la 
				 * posicion del paciente.
				 */
				for(int i = 0; i < pacientes && posPaciente != -1; i++){
					if(listaPacientes[i].atendido == 4){
						posPaciente = i;
					}
				}

				//Si no encuentra un paciente con reaccion
				if(posPacientes == -1){
					/*
					 * Vector que guarda el numero de pacientes en cola 
					 * de cada tipo de paciente.
					 */
					int nPacientesTipo[3];
					for(int i = 0; i < 3; i++){
						nPacientesTipo[i] = 0; 
					}

					/*
					 * Vector que guarda la posicion del paciente mas antiguo
					 * de cada tipo
					 */
					int pacientesAntiguos[3];

					/*
					 * Buscamos el paciente mas antiguo en la cola con mas
					 * pacientes de cada tipo
					 */
					for(int i = pacientes; i >= 0; i--){
						if(listaPacientes[i].atendido == 0){
							nPacientesTipo[listaDePacientes[i].grupoVacunacion] += 1;
							pacientesAntiguos[listaDePacientes[i].grupoVacunacion] = i;
						}
					}

					if(nPacientesTipo[0] >= nPacientesTipo[1] && nPacientesTipo[0] >= nPacientesTipo[2]){
						posPaciente = pacientesAntiguos[0];
					}else if(nPacientesTipo[1] >= nPacientesTipo[0] && nPacientesTipo[1] >= nPacientesTipo[2]){
						posPaciente = pacientesAntiguos[1]
					}else{
						posPaciente = pacientesAntiguos[2];
					}

					//Se le cambia el flag de atendido al paciente si es un paciente para vacunar
					listaPacientes[posPaciente].atendido = 1;
				}else{
					//Se le cambia el flag de atendido al paciente si es un paciente con reaccion
					listaPacientes[posPaciente].atendido = 5;
				}
				
				//Se le cambia el flag de atendido al paciente
				pthread_mutex_unlock(&mutexColaPacientes);
				//Se espera 1 segundo para repetir el bucle si posPaciente es -1
				if(posPaciente == -1){
					sleep(1);
				}
			}
		
		}

		//Medico sale del bucle de buscar pacientes
		
		//Escribe en el fichero que comienza la atencion
		pthread_mutex_lock(&mutexFichero);
		void writeLogMessage("Medico", "Comienza la atencion al paciente nº" + posPaciente);
		pthread_mutex_unlock(&mutexFichero);

		/*
		 * Se Calcula el tipo de atencion y se duerme lo indicado
		 */
		int atencionPaciente = calcularAtencion();
		if(atencionPaciente == 0){
			sleep(calculaRandom(1, 4);
		}else if(atencionPaciente == 1){
			sleep(calculaRandom(2, 6);
		}else{
			sleep(calculaRandom(6, 10);
		}

		//Escribe en el fichero que termina la atencion
		pthread_mutex_lock(&mutexFichero);
                void writeLogMessage("Medico", "Termina la atencion al paciente nº" + posPaciente);
                pthread_mutex_unlock(&mutexFichero);

		//Accedemos a la cola para cambiar el flag de atendido
		pthread_mutex_lock(&mutexColaPacientes);
		if(atencionPaciente == 0 || atencionPaciente == 1){
			listaPacientes[posPaciente].atendido = 4;
		}else{
			listaPacientes[posPaciente].atendido = -1;
		}
		pthread_mutex_lock(&mutexColaPacientes);
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
                int duerme;

                for(int i = 0; i < MAXPACIENTES; i++) {
                    if(enfermero1.atendiendo == 0 &&listaPacientes[i].tipo == 0 && listaPacientes[i].atendido == 0) {  //Comprobamos si hay del mismo tipo, si ha sido atendido y si ese enfermero esta atendiendo
                        enfermero1.atendiendo = 1;
                        enfermero1.pacientesAtendidos++;

                        int aleatorio = calculaRandom(0, 100);

                        if(aleatorio < 80) {
                            duerme = calculaRandom(1, 4);
                            sleep(duerme);
                            printf("Todo en regla\n");
                            //Comprueba reaccion y estudio
                        }else if(aleatorio < 90) {
                            duerme = calculaRandom(2, 6);
                            sleep(duerme);
                            printf("Mal identificado\n");
                            //Comprueba reaccion y estudio
                        }else {
                            duerme = calculaRandom(6, 10);
                            sleep(duerme);
                            //Aqui sale del consultorio
                        }

                        listaPacientes[i].atendido = 1;//Marcamos el paciente como atendido
                    }
                }

                //No hay pacientes de tipo1, buscamos de otros tipos
                for(int i = 0; i < MAXPACIENTES; i++) {
                    if(enfermero1.atendiendo == 0 && listaPacientes[i].atendido == 0) {  
                        enfermero1.atendiendo = 1;
                        enfermero1.pacientesAtendidos++;

                        int aleatorio = calculaRandom(0, 100);

                        if(aleatorio < 80) {
                            duerme = calculaRandom(1, 4);
                            sleep(duerme);
                            printf("Todo en regla\n");
                            //Comprueba reaccion y estudio
                        }else if(aleatorio < 90) {
                            duerme = calculaRandom(2, 6);
                            sleep(duerme);
                            printf("Mal identificado\n");
                            //Comprueba reaccion y estudio
                        }else {
                            duerme = calculaRandom(6, 10);
                            sleep(duerme);
                            //Aqui sale del consultorio
                        }

                        listaPacientes[i].atendido = 1;//Marcamos el paciente como atendido
                    }
                }

                if(enfermero1.pacientesAtendidos == 5) { //Si es 5 entonces podra descansar
                    enfermero1.atendiendo = 0;//
                    enfermero1.pacientesAtendidos = 0; //Resetemaos el contador de pacientes para que pueda volver a empezar
                    sleep(5); //Descansa sus 5 segundos 
                    //Aqui creo que habra que indicar a otro enfermero o al medico que debe vacunar
                }

                break;
            case 1:
                printf("Soy el enfermer@_%d", grupoVacunacion + 1); //Asignamos al enfermero su identificador secuencial

                for(int i = 0; i < MAXPACIENTES; i++) {
                    if(enfermero2.atendiendo == 0 &&listaPacientes[i].tipo == 0 && listaPacientes[i].atendido == 0) {  //Comprobamos si hay del mismo tipo, si ha sido atendido y si ese enfermero esta atendiendo
                        enfermero2.atendiendo = 1;
                        enfermero2.pacientesAtendidos++;

                        int aleatorio = calculaRandom(0, 100);

                        if(aleatorio < 80) {
                            duerme = calculaRandom(1, 4);
                            sleep(duerme);
                            printf("Todo en regla\n");
                            //Comprueba reaccion y estudio
                        }else if(aleatorio < 90) {
                            duerme = calculaRandom(2, 6);
                            sleep(duerme);
                            printf("Mal identificado\n");
                            //Comprueba reaccion y estudio
                        }else {
                            duerme = calculaRandom(6, 10);
                            sleep(duerme);
                            //Aqui sale del consultorio
                        }

                        listaPacientes[i].atendido = 1;//Marcamos el paciente como atendido
                    }
                }

                //No hay pacientes de tipo1, buscamos de otros tipos
                for(int i = 0; i < MAXPACIENTES; i++) {
                    if(enfermero2.atendiendo == 0 && listaPacientes[i].atendido == 0) {  
                        enfermero2.atendiendo = 1;
                        enfermero2.pacientesAtendidos++;

                        int aleatorio = calculaRandom(0, 100);

                        if(aleatorio < 80) {
                            duerme = calculaRandom(1, 4);
                            sleep(duerme);
                            printf("Todo en regla\n");
                            //Comprueba reaccion y estudio
                        }else if(aleatorio < 90) {
                            duerme = calculaRandom(2, 6);
                            sleep(duerme);
                            printf("Mal identificado\n");
                            //Comprueba reaccion y estudio
                        }else {
                            duerme = calculaRandom(6, 10);
                            sleep(duerme);
                            //Aqui sale del consultorio
                        }

                        listaPacientes[i].atendido = 1;//Marcamos el paciente como atendido
                    }
                }

                if(enfermero2.pacientesAtendidos == 5) { //Si es 5 entonces podra descansar
                    enfermero2.atendiendo = 0;//
                    enfermero2.pacientesAtendidos = 0; //Resetemaos el contador de pacientes para que pueda volver a empezar
                    sleep(5); //Descansa sus 5 segundos 
                    //Aqui creo que habra que indicar a otro enfermero o al medico que debe vacunar
                }

                break;

            default:
                printf("Soy el enfermer@_%d", grupoVacunacion + 1); //Asignamos al enfermero su identificador secuencial

                for(int i = 0; i < MAXPACIENTES; i++) {
                    if(enfermero2.atendiendo == 0 &&listaPacientes[i].tipo == 0 && listaPacientes[i].atendido == 0) {  //Comprobamos si hay del mismo tipo, si ha sido atendido y si ese enfermero esta atendiendo
                        enfermero2.atendiendo = 1;
                        enfermero2.pacientesAtendidos++;

                        int aleatorio = calculaRandom(0, 100);

                        if(aleatorio < 80) {
                            duerme = calculaRandom(1, 4);
                            sleep(duerme);
                            printf("Todo en regla\n");
                            //Comprueba reaccion y estudio
                        }else if(aleatorio < 90) {
                            duerme = calculaRandom(2, 6);
                            sleep(duerme);
                            printf("Mal identificado\n");
                            //Comprueba reaccion y estudio
                        }else {
                            duerme = calculaRandom(6, 10);
                            sleep(duerme);
                            //Aqui sale del consultorio
                        }

                        listaPacientes[i].atendido = 1;//Marcamos el paciente como atendido
                    }
                }

                //No hay pacientes de tipo1, buscamos de otros tipos
                for(int i = 0; i < MAXPACIENTES; i++) {
                    if(enfermero2.atendiendo == 0 && listaPacientes[i].atendido == 0) {  
                        enfermero2.atendiendo = 1;
                        enfermero2.pacientesAtendidos++;

                        int aleatorio = calculaRandom(0, 100);

                        if(aleatorio < 80) {
                            duerme = calculaRandom(1, 4);
                            sleep(duerme);
                            printf("Todo en regla\n");
                            //Comprueba reaccion y estudio
                        }else if(aleatorio < 90) {
                            duerme = calculaRandom(2, 6);
                            sleep(duerme);
                            printf("Mal identificado\n");
                            //Comprueba reaccion y estudio
                        }else {
                            duerme = calculaRandom(6, 10);
                            sleep(duerme);
                            //Aqui sale del consultorio
                        }

                        listaPacientes[i].atendido = 1;//Marcamos el paciente como atendido
                    }
                }

                if(enfermero2.pacientesAtendidos == 5) { //Si es 5 entonces podra descansar
                    enfermero2.atendiendo = 0;//
                    enfermero2.pacientesAtendidos = 0; //Resetemaos el contador de pacientes para que pueda volver a empezar
                    sleep(5); //Descansa sus 5 segundos 
                    //Aqui creo que habra que indicar a otro enfermero o al medico que debe vacunar
                }

                break;
        }
    }
}

/*
 * Hilo que representa al Estadístico
 */
void *hiloEstadistico(void *arg){
	
}

int calculaRandom(int n1, int n2){
    return (rand() % (n2-n1+1)) + n1;
}

/*
 *Metodo que calcula el tipo de atencion
 *Devuelve 0 si esta todo en orden
 *         1 si no se ha identificado correctamente
 *         2 si tiene catarro o gripe
 *Para el paciente
 */
int calcularAtencion(){
	int numeroAleatorio = calculaRandom(1, 100);
	if(numeroAleatorio < 80){
		return 0;
	}else if(numeroAleatorio < 90){
		return 1;
	}else{
		return 2;
	}
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
