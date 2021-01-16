#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>


pthread_mutex_t mutexFichero, mutexColaPacientes;
pthread_cond_t varEstadistico,varPacientes;
int contadorPacientes; 
const int MAXPACIENTES=15;
struct Paciente
{
    int id;// Identificacion del paciente

    /*
     * -1 - si ya se pueden marchar
     *  0 - si no ha sido atendido
     *  1 - si esta siendo atendido para vacunar
     *  2 - si ha sido vacunado
     *  4 - si ha dado reaccion
     *  5 - si esta siendo atendido por el medico despues de dar reaccion
     */
    int atendido;
    
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
    int atendiendo; //0 si esta libre, 1 si esta atendiendo a un paciente
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
	signal(SIGUSR1, nuevoPaciente);

//2. signal o sigaction SIGUSR2, paciente medio.
	signal(SIGUSR2, nuevoPaciente);

//3. signal o sigaction SIGPIPE, paciente senior.
	signal(SIGPIPE, nuevoPaciente);

//4. signal o sigaction SIGINT, terminar
    signal(SIGINT, SIG_DFL); // La señal por defecto de SIGINT es suspender la ejecución
//5. Inicializar recursos (¡Ojo!, Inicializar!=Declarar).
    //a. Semáforos.
    if (pthread_mutex_init(&mutexFichero, NULL)!=0){
        exit(-1);
    }
    if (pthread_mutex_init(&mutexColaPacientes, NULL)!=0){
        exit(-1);
    }  
    //b. Contador de pacientes.
    contadorPacientes=0;

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

    logFile = fopen ("registroTiempos.log", "w");

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
    while (1)
    {
        pause();
    }
    
}

void nuevoPaciente(int tipo){
    /**
    1. Comprobar si hay espacio en la lista de pacientes.
        a. Si lo hay
            i. Se añade el paciente.
            ii. Contador de pacientes se incrementa.
            iii. nuevaPaciente.id = ContadorPacientes.
            iv. nuevoPaciente.atendido=0
            v. tipo=Depende de la señal recibida.
            vi. nuevoPaciente.Serología=0.
            vii. Creamos hilo para el paciente.
        b. Si no hay espacio
            i. Se ignora la llamada.
    */
}



void *hiloPaciente (void *arg) {
    int comportamiento;
    char type[20];
    char mensaje[50];
    switch(Paciente.tipo){
    	case 0:
    	sprintf(type, "%s","Junior");
    	break;
    	case 1:
    	sprintf(type, "%s","Medio");
    	break;
    	case 2:
    	sprintf(type, "%s","Senior");
    	break;
    	default:
    	sprintf(type, "%s","Desconocido");
    	break;
    }
    sprintf(mensaje,"Entra Paciente de tipo: %s", type);
    pthread_mutex_lock(&mutexFichero);
    writeLogMessage(type, mensaje);
    pthread_mutex_unlock(&mutexFichero);
    sleep(3);
    if(Paciente.atendido==1){
        printf("El paciente: %s esta siendo atentido\n", Paciente.id);
    }else{
        printf("El paciente: %s no esta siendo atentido\n",Paciente.id);
        while(paciente.atendido==0){
        	comportamiento=calculaRandom(1,10);
        	if(comportamiento<=3){
        		sprintf(mensaje,"El paciente: %s abandona la consulta\n", paciente.id);
        		pthread_mutex_lock(&mutexFichero);
    			writeLogMessage(comportamiento, mensaje);
    			pthread_mutex_unlock(&mutexFichero);
        		paciente=NULL;
        		contadorPacientes --;
        		pthread_exit;
        	}else{
        		comportamiento=calculaRandom(1,100);
        		if(comportamiento<=5){
        			sprintf(mensaje, "El paciente: %s se va al baño y pierde su turno.\n", paciente.id);
        			pthread_mutex_lock(&mutexFichero);
    				writeLogMessage(comportamiento, mensaje);
    				pthread_mutex_unlock(&mutexFichero);
        			paciente=NULL;
        			contadorPacientes --;
        			pthread_exit;
        		}else{
        			sprintf(mensaje, "El paciente: %s decide esperar a su turno", paciente.id);
        			sleep(3);
        		}
        	}
        }
    }
}

/*
 * Hilo que representa al médico
 */
void *hiloMedico(void *arg){

	//Variable que guarda la posición del paciente que se busca
        int posPaciente;
	/*0 - si no es un paciente con reaccion
         *1 - si es un paciente con reaccion
         */
	int reaccion;
	//Se ejecuta indefinidamente hasta que se recibe el la señal
	while(1){
		//Variable que guarda la posición del paciente que se busca
		posPaciente = -1;
		while(posPaciente == -1){
			if(contadorPacientes > 0){
				//Como accedemos a la lista bloqueamos el mutex
				pthread_mutex_lock(&mutexColaPacientes);
				/*
				 * Buscamos el paciente con reaccion que mas tiempo lleve esperando
				 * Si no hay posPaciente seguira siendo -1 y si hay pasa a ser la 
				 * posicion del paciente.
				 */
				for(int i = 0; i < contadorPacientes && posPaciente != -1; i++){
					if(listaPacientes[i].atendido == 4){
						posPaciente = i;
						reaccion = 1;
					}
				}

				//Si no encuentra un paciente con reaccion
				if(posPaciente == -1){
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
					for(int i = 0; i < 3; i++){
						pacientesAntiguos[i] = -1;
					}
					/*
					 * Buscamos el paciente mas antiguo en la cola con mas
					 * pacientes de cada tipo
					 */
					for(int i = contadorPacientes; i >= 0; i--){
						if(listaPacientes[i].atendido == 0){
							nPacientesTipo[listaPacientes[i].tipo] += 1;
							pacientesAntiguos[listaPacientes[i].tipo] = i;
						}
					}

					//Si se ha encontrado algun paciente antiguo
					if(pacientesAntiguos[1] != -1 || pacientesAntiguos[2] != -1 || pacientesAntiguos[0] != -1){
						if(nPacientesTipo[0] >= nPacientesTipo[1] && nPacientesTipo[0] >= nPacientesTipo[2]){
							posPaciente = pacientesAntiguos[0];
						}else if(nPacientesTipo[1] >= nPacientesTipo[0] && nPacientesTipo[1] >= nPacientesTipo[2]){
							posPaciente = pacientesAntiguos[1];
						}else{
							posPaciente = pacientesAntiguos[2];
						}

						reaccion = 0;
						//Se le cambia el flag de atendido al paciente si es un paciente para vacunar
                        	                listaPacientes[posPaciente].atendido = 1;
			
					}
				}else{
					//Se le cambia el flag de atendido al paciente si tiene reaccion
					listaPacientes[posPaciente].atendido = 5;
				}
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
		writeLogMessage("Medico", "Comienza la atencion al paciente nº" + posPaciente);
		pthread_mutex_unlock(&mutexFichero);
		if(reaccion){
			/*
			 * Se Calcula el tipo de atencion y se duerme lo indicado
			 */
			int atencionPaciente = calcularAtencion();
			if(atencionPaciente == 0){
				sleep(calculaRandom(1, 4));
			}else if(atencionPaciente == 1){
				sleep(calculaRandom(2, 6));
			}else{
				sleep(calculaRandom(6, 10));
			}

			//Escribe en el fichero que termina la atencion
			pthread_mutex_lock(&mutexFichero);
                	writeLogMessage("Medico", "Termina la atencion al paciente nº" + posPaciente);
                	pthread_mutex_unlock(&mutexFichero);

			//Accedemos a la cola para cambiar el flag de atendido
			pthread_mutex_lock(&mutexColaPacientes);
			if(atencionPaciente == 0 || atencionPaciente == 1){
				listaPacientes[posPaciente].atendido = 3;
			}else{
				listaPacientes[posPaciente].atendido = -1;
			}
			pthread_mutex_lock(&mutexColaPacientes);
		}else{
			sleep(5);
			pthread_mutex_lock
		}
		//Escribe en el fichero que termina la atencion
                pthread_mutex_lock(&mutexFichero);
                writeLogMessage("Medico", "Termina la atencion al paciente nº" + posPaciente);
                pthread_mutex_unlock(&mutexFichero);
    }	
}

/*
 * Hilo que representa al Enfermrer@
 */
void *hiloEnfermero(void *arg) {
    while(1) {
        switch(grupoVacunacion) { //Sabiendo el grupo al que  vacuna realizara buscara en un sitio u otro
            case 0: 
                pthread_mutex_lock(&mutexColaPacientes); //Bloqueamos lista para acceder al mutex

                printf("Soy el enfermer@_%d", grupoVacunacion + 1); //Asignamos al enfermero su identificador secuencial
                int duerme;

                for(int i = 0; i < contadorPacientes; i++) {
                    if(enfermero1.atendiendo == 0 &&listaPacientes[i].tipo == 0 && listaPacientes[i].atendido == 0) {  //Comprobamos si hay del mismo tipo, si ha sido atendido y si ese enfermero esta atendiendo
                        enfermero1.atendiendo = 1;
                        enfermero1.pacientesAtendidos++;

                        int aleatorio = calculaRandom(0, 100);

                        pthread_mutex_lock(&mutexFichero);
                        writeLogMessage("Enfermero", "Comienza la atencion al paciente nº" + i);
                        pthread_mutex_unlock(&mutexFichero);

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

                        if(enfermero1.pacientesAtendidos == 5) { //Si es 5 entonces podra descansar
                            enfermero1.atendiendo = 0;//
                            enfermero1.pacientesAtendidos = 0; //Resetemaos el contador de pacientes para que pueda volver a empezar
                            sleep(5); //Descansa sus 5 segundos 
                            //Aqui creo que habra que indicar a otro enfermero o al medico que debe vacunar
                        }

                        pthread_mutex_lock(&mutexFichero);
                        writeLogMessage("Enfermero", "Termina la atencion al paciente nº" + i);
                        pthread_mutex_unlock(&mutexFichero);

                        listaPacientes[i].atendido = 1;//Marcamos el paciente como atendido
                        pthread_mutex_unlock(&mutexColaPacientes); //Como ya hemos atendido al paciente desbloquamos la cola
                    }
                }

                //No hay pacientes de tipo1, buscamos de otros tipos
                for(int i = 0; i < contadorPacientes; i++) {
                    if(enfermero1.atendiendo == 0 && listaPacientes[i].atendido == 0) {  
                        enfermero1.atendiendo = 1;
                        enfermero1.pacientesAtendidos++;

                        int aleatorio = calculaRandom(0, 100);

                        pthread_mutex_lock(&mutexFichero);
                        writeLogMessage("Enfermero", "Comienza la atencion al paciente nº" + i);
                        pthread_mutex_unlock(&mutexFichero);

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

                        if(enfermero1.pacientesAtendidos == 5) { //Si es 5 entonces podra descansar
                            enfermero1.atendiendo = 0;//
                            enfermero1.pacientesAtendidos = 0; //Resetemaos el contador de pacientes para que pueda volver a empezar
                            sleep(5); //Descansa sus 5 segundos 
                            //Aqui creo que habra que indicar a otro enfermero o al medico que debe vacunar
                        }

                        pthread_mutex_lock(&mutexFichero);
                        writeLogMessage("Enfermero", "Termina la atencion al paciente nº" + i);
                        pthread_mutex_unlock(&mutexFichero);

                        listaPacientes[i].atendido = 1;//Marcamos el paciente como atendido
                        pthread_mutex_unlock(&mutexColaPacientes); //Como ya hemos atendido al paciente desbloquamos la cola

                    }
                }

                //No ha encontrados pacientes, entonces libera mutex y duerme un sec para volver a empezar a buscar
                pthread_mutex_unlock(&mutexColaPacientes);
                sleep(1);

                break;
            case 1:
                printf("Soy el enfermer@_%d", grupoVacunacion + 1); //Asignamos al enfermero su identificador secuencial

                pthread_mutex_lock(&mutexColaPacientes); //Bloqueamos lista para acceder al mutex

                for(int i = 0; i < contadorPacientes; i++) {
                    if(enfermero2.atendiendo == 0 &&listaPacientes[i].tipo == 0 && listaPacientes[i].atendido == 0) {  //Comprobamos si hay del mismo tipo, si ha sido atendido y si ese enfermero esta atendiendo
                        enfermero2.atendiendo = 1;
                        enfermero2.pacientesAtendidos++;

                        int aleatorio = calculaRandom(0, 100);

                        pthread_mutex_lock(&mutexFichero);
                        writeLogMessage("Enfermero", "Comienza la atencion al paciente nº" + i);
                        pthread_mutex_unlock(&mutexFichero);

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

                        if(enfermero2.pacientesAtendidos == 5) { //Si es 5 entonces podra descansar
                            enfermero2.atendiendo = 0;//
                            enfermero2.pacientesAtendidos = 0; //Resetemaos el contador de pacientes para que pueda volver a empezar
                            sleep(5); //Descansa sus 5 segundos 
                            //Aqui creo que habra que indicar a otro enfermero o al medico que debe vacunar
                        }

                        pthread_mutex_lock(&mutexFichero);
                        writeLogMessage("Enfermero", "Termina la atencion al paciente nº" + i);
                        pthread_mutex_unlock(&mutexFichero);

                        listaPacientes[i].atendido = 1;//Marcamos el paciente como atendido
                        pthread_mutex_unlock(&mutexColaPacientes); //Como ya hemos atendido al paciente desbloquamos la cola

                    }
                }

                //No hay pacientes de tipo1, buscamos de otros tipos
                for(int i = 0; i < contadorPacientes; i++) {
                    if(enfermero2.atendiendo == 0 && listaPacientes[i].atendido == 0) {  
                        enfermero2.atendiendo = 1;
                        enfermero2.pacientesAtendidos++;

                        int aleatorio = calculaRandom(0, 100);

                        pthread_mutex_lock(&mutexFichero);
                        writeLogMessage("Enfermero", "Comienza la atencion al paciente nº" + i);
                        pthread_mutex_unlock(&mutexFichero);

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

                        if(enfermero2.pacientesAtendidos == 5) { //Si es 5 entonces podra descansar
                            enfermero2.atendiendo = 0;//
                            enfermero2.pacientesAtendidos = 0; //Resetemaos el contador de pacientes para que pueda volver a empezar
                            sleep(5); //Descansa sus 5 segundos 
                            //Aqui creo que habra que indicar a otro enfermero o al medico que debe vacunar
                        }

                        pthread_mutex_lock(&mutexFichero);
                        writeLogMessage("Enfermero", "Termina la atencion al paciente nº" + i);
                        pthread_mutex_unlock(&mutexFichero);

                        listaPacientes[i].atendido = 1;//Marcamos el paciente como atendido
                        pthread_mutex_unlock(&mutexColaPacientes); //Como ya hemos atendido al paciente desbloquamos la cola
                    }
                }

                //No ha encontrados pacientes, entonces libera mutex y duerme un sec para volver a empezar a buscar
                pthread_mutex_unlock(&mutexColaPacientes);
                sleep(1);

                break;

            default:
                printf("Soy el enfermer@_%d", grupoVacunacion + 1); //Asignamos al enfermero su identificador secuencial

                for(int i = 0; i < contadorPacientes; i++) {
                    if(enfermero3.atendiendo == 0 &&listaPacientes[i].tipo == 0 && listaPacientes[i].atendido == 0) {  //Comprobamos si hay del mismo tipo, si ha sido atendido y si ese enfermero esta atendiendo
                        enfermero3.atendiendo = 1;
                        enfermero3.pacientesAtendidos++;

                        int aleatorio = calculaRandom(0, 100);

                        pthread_mutex_lock(&mutexFichero);
                        writeLogMessage("Enfermero", "Comienza la atencion al paciente nº" + i);
                        pthread_mutex_unlock(&mutexFichero);

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

                        if(enfermero3.pacientesAtendidos == 5) { //Si es 5 entonces podra descansar
                            enfermero3.atendiendo = 0;//
                            enfermero3.pacientesAtendidos = 0; //Resetemaos el contador de pacientes para que pueda volver a empezar
                            sleep(5); //Descansa sus 5 segundos 
                            //Aqui creo que habra que indicar a otro enfermero o al medico que debe vacunar
                        }

                        pthread_mutex_lock(&mutexFichero);
                        writeLogMessage("Enfermero", "Termina la atencion al paciente nº" + i);
                        pthread_mutex_unlock(&mutexFichero);

                        listaPacientes[i].atendido = 1;//Marcamos el paciente como atendido
                        pthread_mutex_unlock(&mutexColaPacientes); //Como ya hemos atendido al paciente desbloquamos la cola
                    }
                }

                //No hay pacientes de tipo1, buscamos de otros tipos
                for(int i = 0; i < contadorPacientes; i++) {
                    if(enfermero3.atendiendo == 0 && listaPacientes[i].atendido == 0) {  
                        enfermero3.atendiendo = 1;
                        enfermero3.pacientesAtendidos++;

                        int aleatorio = calculaRandom(0, 100);

                        pthread_mutex_lock(&mutexFichero);
                        writeLogMessage("Enfermero", "Comienza la atencion al paciente nº" + i);
                        pthread_mutex_unlock(&mutexFichero);

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

                        if(enfermero3.pacientesAtendidos == 5) { //Si es 5 entonces podra descansar
                            enfermero3.atendiendo = 0;//
                            enfermero3.pacientesAtendidos = 0; //Resetemaos el contador de pacientes para que pueda volver a empezar
                            sleep(5); //Descansa sus 5 segundos 
                            //Aqui creo que habra que indicar a otro enfermero o al medico que debe vacunar
                        }

                        pthread_mutex_lock(&mutexFichero);
                        writeLogMessage("Enfermero", "Termina la atencion al paciente nº" + i);
                        pthread_mutex_unlock(&mutexFichero);

                        listaPacientes[i].atendido = 1;//Marcamos el paciente como atendido
                        pthread_mutex_unlock(&mutexColaPacientes); //Como ya hemos atendido al paciente desbloquamos la cola
                    }
                }

                //No ha encontrados pacientes, entonces libera mutex y duerme un sec para volver a empezar a buscar
                pthread_mutex_unlock(&mutexColaPacientes);
                sleep(1);

                break;
        }
    }
}

/*
 * Hilo que representa al Estadístico
 */
void *hiloEstadistico(void *arg){
	while(1){
		pthread_mutex_lock(/*mutex*/);
		pthread_cond_wait(&varEstadistico, /*mutex*/);
		pthread_mutex_lock(&mutexFichero);
		writeLogMessage("Estadistico", "Comienza la actividad");
		pthread_mutex_unlock(&mutexFichero);
		sleep(4);
		pthread_mutex_lock(&mutexFichero);
                writeLogMessage("Estadistico", "Comienza la actividad");               
                pthread_mutex_unlock(&mutexFichero);
		pthread_cond_signal(&varPacientes);
		pthread_mutex_unlock(/*mutex*/);

	}
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
