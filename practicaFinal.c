#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

// Compilar con -lpthread

pthread_mutex_t mutexFichero, mutexColaPacientes;
pthread_cond_t varEstadistico,varPacientes;
int contadorPacientes;
#define MAXPACIENTES 15
struct Paciente
{
    int id;// Identificacion del paciente

    /*
     * -1 - si ya se puede marchar
     *  0 - si no ha sido atendido
     *  1 - si esta siendo atendido para vacunar
     *  2 - si ha sido vacunado y tenia los papeles en orden
     *  3 - si se ha vacunado pero no tenia los papeles en orden
     *  4 - si ha dado reaccion
     *  5 - si esta siendo atendido por el medico despues de dar reaccion
     *  6 - si tiene gripe
     *  7 - si puede entrar al estudio del estadistico
     *  8 - si esta realizando la encuesta
     */
    int atendido;
    
    /**
     * Junior(0-16 años): 0
     * Medios(16-60 años): 1
     * Senior(60+ años): 2
     */ 
    int tipo;
    int serologia; // 0 si no participa 1 en caso contrario
    struct Paciente *ant;//Paciente anterior
    struct Paciente *sig;// Paciente siguiente
};
struct Paciente *primerPaciente;
struct Paciente *ultimoPaciente;

struct Enfermero
{
    int id;
    /**
     * Junior(0-16 años): 0
     * Medios(16-60 años): 1
     * Senior(60+ años): 2
     */ 
    int grupoVacunacion; 
    int atendiendo; //0 si esta libre, 1 si esta atendiendo a un paciente
    int pacientesAtendidos;
};
struct Enfermero enfermero1,enfermero2,enfermero3;

pthread_t medico, estadistico;
pthread_t threadEnfermero1, threadEnfermero2, threadEnfermero3;

char logFileName[]="registroTiempos.log";
FILE *logFile;
int i=0;

/*
 *Declaracion de funciones a utilizar
 */
int calculaRandom(int n1, int n2);
void writeLogMessage(char *id, char *msg);
void nuevoPaciente(int tipo);
int calcularAtencion();
void *hiloPaciente (void *arg);
void *hiloMedico(void *arg);
void *hiloEnfermero(void *arg);
void *hiloEstadistico(void *arg);
void eliminarPaciente(struct Paciente **pacienteAEliminar);

int main(int argc, char argv[]){ //TODO terminar programa cuando se hallan atendido a todos los pacientes y se halla recibido la señal SIGINT
//1. signal o sigaction SIGUSR1, paciente junior.
    srand (time(NULL));
	if(signal(SIGUSR1, &nuevoPaciente) == SIG_ERR){
		perror("Llamada a signal");
		exit(-1);
	}

//2. signal o sigaction SIGUSR2, paciente medio.
    	if(signal(SIGUSR2, &nuevoPaciente) == SIG_ERR){
		perror("Llamada a signal");
        	exit(-1);
    }
//3. signal o sigaction SIGPIPE, paciente senior.
	if(signal(SIGPIPE, &nuevoPaciente) == SIG_ERR){
		perror("Llamada a signal");
		exit(-1);
    }

//4. signal o sigaction SIGINT, terminar
    if(signal(SIGINT, SIG_DFL) == SIG_ERR){
        perror("Llamada a signal");
        exit(-1);
    } // La señal por defecto de SIGINT es suspender la ejecución
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
    //for (size_t i = 0; i < MAXPACIENTES; i++)
    //{
    //    listaPacientes[i].atendido=0;
    //    listaPacientes[i]->id=0;
    //    listaPacientes[i].serologia=0;
    //    listaPacientes[i].tipo=0;
    //}
    

    //d. Lista de enfermer@s (si se incluye).
    enfermero1.atendiendo=0;
    enfermero1.grupoVacunacion=0;
    enfermero1.id=1;
    enfermero1.pacientesAtendidos=0;

    enfermero2.atendiendo=0;
    enfermero2.grupoVacunacion=1;
    enfermero2.id=2;
    enfermero2.pacientesAtendidos=0;

    enfermero3.atendiendo=0;
    enfermero3.grupoVacunacion=2;
    enfermero3.id=3;
    enfermero3.pacientesAtendidos=0;

    //e. Fichero de Log

    logFile = fopen (logFileName, "w");

    //f. Variables condición
    if (pthread_cond_init(&varEstadistico, NULL)!=0){
        exit(-1); 
    } 
    if (pthread_cond_init(&varPacientes, NULL)!=0){
        exit(-1); 
    } 
//6. Crear 3 hilos enfermer@s.
    int n1 = 0, n2 = 1, n3 = 2;
    pthread_create (&threadEnfermero1, NULL, hiloEnfermero, (void *)&n1);
    pthread_create (&threadEnfermero2, NULL, hiloEnfermero, (void *)&n2);
    pthread_create (&threadEnfermero3, NULL, hiloEnfermero, (void *)&n3);
//7. Crear el hilo médico.
    //pthread_create (&medico, NULL, hiloMedico, NULL);
//8. Crear el hilo estadístico.
    pthread_create (&estadistico, NULL, hiloEstadistico, NULL);
//9. Esperar por señales de forma infinita.
    while (1){
        
	    pause();
	    printf("Bucle %d: Esperando\n",i);
        i++;
    }
    printf("Bucle %d: Terminado\n",i);
}




void nuevoPaciente(int tipo){
    printf("entre");
    if(signal(tipo, nuevoPaciente) == SIG_ERR){
	    perror("Llamada a signal");
	    exit(-1);
    } 
   //1. Comprobar si hay espacio en la lista de pacientes.
    pthread_mutex_lock(&mutexColaPacientes);
    if (contadorPacientes<MAXPACIENTES){//a. Si lo hay
        //i. Se añade el paciente.
        struct Paciente *pacienteNuevo;
        pacienteNuevo=malloc(sizeof *pacienteNuevo);
        //ii. Contador de pacientes se incrementa.
        contadorPacientes++;

        //iii. nuevaPaciente->id = ContadorPacientes.
        pacienteNuevo->id=contadorPacientes;

        //iv. nuevoPaciente.atendido=0
        pacienteNuevo->atendido=0;

	    pacienteNuevo->sig=NULL;
	    pacienteNuevo->ant=NULL;

        //v. tipo=Depende de la señal recibida.
        if (tipo == SIGUSR1){
            pacienteNuevo->tipo=0;
        }

        if (tipo == SIGUSR2)
        {
            pacienteNuevo->tipo=1;
        }

        if (tipo == SIGPIPE)
        {
            pacienteNuevo->tipo=2;
        }
        
        //vi. nuevoPaciente.Serología=0.
        pacienteNuevo->serologia=0;

        //vii. Creamos hilo para el paciente.
        pthread_t threadNuevoPaciente;
        if (primerPaciente==NULL){
            primerPaciente=pacienteNuevo;
            ultimoPaciente = primerPaciente;
        }else{
            pacienteNuevo->ant = ultimoPaciente;
            ultimoPaciente->sig = pacienteNuevo;
            ultimoPaciente = pacienteNuevo;
        }
        pthread_create (&threadNuevoPaciente, NULL, hiloPaciente, NULL);
        
        
        

    }
    pthread_mutex_unlock(&mutexColaPacientes);
}



void *hiloPaciente (void *arg) {
    printf("aaaaaaa");
    struct Paciente *paciente;
    int atendido;
    int comportamiento;
    char type[100];
    char mensaje[100];
    pthread_mutex_lock(&mutexColaPacientes);
    paciente = ultimoPaciente;
    switch(paciente->tipo){
    	case 0:
    	sprintf(type, "%s%d %s","Paciente ", paciente->id, "Junior");
    	break;
    	case 1:
    	sprintf(type, "%s%d %s","Paciente ", paciente->id, "Medio");
    	break;
    	case 2:
    	sprintf(type, "%s%d %s","Paciente ", paciente->id, "Senior");
    	break;
    	default:
    	sprintf(type, "%s%d %s","Paciente ", paciente->id, "Desconocido");
    	break;
    }
    pthread_mutex_unlock(&mutexColaPacientes);

    sprintf(mensaje, "Entra el Paciente");
    pthread_mutex_lock(&mutexFichero);
    writeLogMessage(type, mensaje);
    pthread_mutex_unlock(&mutexFichero);

    //sleep(3);
    
    if(atendido==1){
        printf("El paciente: %d esta siendo atentido\n", paciente->id);
    }else{
        printf("El paciente: %d no esta siendo atentido\n",paciente->id);
        do{
            pthread_mutex_lock(&mutexColaPacientes);
            atendido = paciente->atendido;
            pthread_mutex_unlock(&mutexColaPacientes);
            comportamiento=calculaRandom(1,10);
            printf("random(1)%d\n",comportamiento);
            if(atendido == 1){
                printf("El paciente: %d esta siendo atentido\n", paciente->id);
            }else{
                if(comportamiento<=3){
                    sprintf(mensaje,"El paciente: %d abandona la consulta\n", paciente->id);
                    pthread_mutex_lock(&mutexFichero);
                    writeLogMessage(type, mensaje);
                    pthread_mutex_unlock(&mutexFichero);

                    pthread_mutex_lock(&mutexColaPacientes);
                    eliminarPaciente(&paciente);
                    free(paciente);
                    contadorPacientes --;
                    pthread_mutex_unlock(&mutexColaPacientes);
                    pthread_exit(NULL);
                }else{
                    comportamiento=calculaRandom(1,100);
                    printf("random(2)%d\n",comportamiento);

                    // Va al baño y pierde el turno
                    if(comportamiento<=5){
                        sprintf(mensaje, "El paciente: %d se va al baño y pierde su turno.\n", paciente->id);
                        pthread_mutex_lock(&mutexFichero);
                        writeLogMessage(type, mensaje);
                        pthread_mutex_unlock(&mutexFichero);
			            pthread_mutex_lock(&mutexColaPacientes);
                        eliminarPaciente(&paciente);
                        free(paciente);
                        contadorPacientes --;                     
                        pthread_mutex_unlock(&mutexColaPacientes);
                        pthread_exit(NULL);
			            printf("Sigo aqui");

                    }else{
                        sprintf(mensaje, "El paciente: %d decide esperar a su turno", paciente->id);
                        pthread_mutex_lock(&mutexFichero);
                        writeLogMessage(type, mensaje);
                        pthread_mutex_unlock(&mutexFichero);
                        sleep(3);
                    }
                }
            }   	
        }while(atendido == 0);
        //compruebo si el paciente tiene gripe.
        if(atendido==6){
        	sprintf(mensaje,"El paciente: %d tiene gripe.", paciente->id);
        	pthread_mutex_lock(&mutexColaPacientes);
            eliminarPaciente(&paciente);
            free(paciente);
        	contadorPacientes --;                                              
 		    pthread_mutex_unlock(&mutexColaPacientes);
        	pthread_exit(NULL);
        }else{
        	//comprueba si da reaccion si da reaccion a la vacuna.
		    pthread_mutex_lock(&mutexColaPacientes);
    		atendido = paciente->atendido;
    		pthread_mutex_unlock(&mutexColaPacientes);

        	if(atendido==4){
        		sprintf(mensaje,"El paciente: %d ha dado reaccion a la vacuna", paciente->id);
        		while(paciente->atendido==5||paciente->atendido==4){
				    pthread_mutex_lock(&mutexColaPacientes);
    				atendido = paciente->atendido;
    				pthread_mutex_unlock(&mutexColaPacientes);
        			sprintf(mensaje, "el paciente: %d esta siendo atendido por el medico", paciente->id);
        			sleep(2);
        		}
        		
        	}else{
			    pthread_mutex_lock(&mutexFichero);
                sprintf(mensaje, "El paciente: %d no ha dado reaccion", paciente->id);
                writeLogMessage(type, mensaje);
    			pthread_mutex_unlock(&mutexFichero);

        		comportamiento = calculaRandom(1,100);
        		if(comportamiento<=25){
        			sprintf(mensaje,"El paciente: %d decide participar en la prueba serologica", paciente->id);
				    pthread_mutex_lock(&mutexColaPacientes);
        			paciente->serologia==1;
				    pthread_mutex_unlock(&mutexColaPacientes);

        			pthread_cond_signal(&varEstadistico);
        			sprintf(mensaje, "El paciente: %d esta preparado para el estudio.\n", paciente->id);
        			pthread_mutex_lock(&mutexFichero);
    				writeLogMessage(type, mensaje);
    				pthread_mutex_unlock(&mutexFichero);

				    pthread_mutex_lock(&mutexColaPacientes);//TODO Crear un nuevo mutex para estadistico?
        			pthread_cond_wait(&varPacientes,&mutexColaPacientes);
				    pthread_mutex_unlock(&mutexColaPacientes);
        			sprintf(mensaje, "El paciente: %d abandona el estudio\n", paciente->id);
        			pthread_mutex_lock(&mutexFichero);
    				writeLogMessage(type, mensaje);
    				pthread_mutex_unlock(&mutexFichero);

        		}else{
        			sprintf(mensaje, "El paciente: %d no va a participar en la prueba serologica", paciente->id);
				    pthread_mutex_lock(&mutexFichero);
                    writeLogMessage(type, mensaje); 
 				    pthread_mutex_unlock(&mutexFichero);

        		}
            }

        }
	}
    sprintf(mensaje, "EL paciente:%d abandona el consultorio", paciente->id);
    pthread_mutex_lock(&mutexFichero);
    writeLogMessage(type, mensaje);
 	pthread_mutex_unlock(&mutexFichero);

	pthread_mutex_lock(&mutexColaPacientes);
	eliminarPaciente(&paciente);
	free(paciente);
   	contadorPacientes --;
	pthread_mutex_unlock(&mutexColaPacientes);

 	pthread_exit(NULL);

  }



/*
 * Hilo que representa al médico
 */
void *hiloMedico(void *arg){

	//Variable que guarda al paciente
	struct Paciente *paciente;
	int contador;
	/*0 - si no es un paciente con reaccion
         *1 - si es un paciente con reaccion
         */
	int reaccion;
	//Se ejecuta indefinidamente hasta que se recibe el la señal
	while(1){
		paciente = NULL;

		while(paciente == NULL){
            pthread_mutex_lock(&mutexColaPacientes);
            contador = contadorPacientes;
            pthread_mutex_unlock(&mutexColaPacientes);
            if(contador > 0){
				//Como accedemos a la lista bloqueamos el mutex
				pthread_mutex_lock(&mutexColaPacientes);
				/*
				 * Buscamos el paciente con reaccion que mas tiempo lleve esperando
				 * Si no hay paciente seguira siendo NULL y si hay pasa a ser la 
				 * posicion del paciente.
				 */

                struct Paciente *sigPaciente;
                sigPaciente = primerPaciente;


				while(sigPaciente != NULL && sigPaciente->sig != NULL && paciente == NULL){
					if(sigPaciente->atendido == 4){
						paciente = sigPaciente;
						reaccion = 1;
					}
					sigPaciente = sigPaciente->sig;

				}

				//Si no encuentra un paciente con reaccion
				if(paciente == NULL){
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

                    contador = contadorPacientes;
					int i = contador - 1;

                    struct Paciente *sigPaciente;
                    sigPaciente = primerPaciente;

					while(sigPaciente != NULL && sigPaciente->sig != NULL){
						if(sigPaciente->atendido == 0){
							nPacientesTipo[sigPaciente->tipo] += 1;
							pacientesAntiguos[sigPaciente->tipo] = i;
						}
						sigPaciente = sigPaciente->sig;
						i--;
					}

					//Si se ha encontrado algun paciente antiguo se le asigna a paciente el paciente mas antiguo
					if(pacientesAntiguos[1] != -1 || pacientesAntiguos[2] != -1 || pacientesAntiguos[0] != -1){
						if(nPacientesTipo[0] >= nPacientesTipo[1] && nPacientesTipo[0] >= nPacientesTipo[2]){
							int i = pacientesAntiguos[0];

                            struct Paciente *sigPaciente;
                            sigPaciente = primerPaciente;
							while(i > 0){
								sigPaciente = sigPaciente->sig;
							}
							paciente = sigPaciente;
						}else if(nPacientesTipo[1] >= nPacientesTipo[0] && nPacientesTipo[1] >= nPacientesTipo[2]){
							int i = pacientesAntiguos[1];
                            struct Paciente *sigPaciente;
                            sigPaciente = primerPaciente;
                            while(i > 0){                  
                                sigPaciente = sigPaciente->sig;
								i--;
                            }
                            paciente = sigPaciente;
						}else{
							int i = pacientesAntiguos[2];
                            struct Paciente *sigPaciente;
                            sigPaciente = primerPaciente;
                            while(i > 0){                  
                                sigPaciente = sigPaciente->sig;
                                i--;
                            }
                            paciente = sigPaciente;
						}

						reaccion = 0;
						//Se le cambia el flag de atendido al paciente si es un paciente para vacunar
                        paciente->atendido = 1;
			
					}
				}else{
					//Se le cambia el flag de atendido al paciente si tiene reaccion
					paciente->atendido = 5;
				}

				//Liberamos el mutex una vez terminamos las operaciones con la cola de pacientes
				pthread_mutex_unlock(&mutexColaPacientes);
				//Se espera 1 segundo para repetir el bucle si posPaciente es -1
				if(paciente == NULL){
					sleep(1);
				}
			}
		
		}

		//Medico sale del bucle de buscar pacientes
		char mensaje[100];

		sprintf(mensaje, "Comienza la atencion al paciente nº%d", paciente->id);
		//Escribe en el fichero que comienza la atencion
		pthread_mutex_lock(&mutexFichero);
		writeLogMessage("Medico", mensaje);
		pthread_mutex_unlock(&mutexFichero);
		if(reaccion == 0){
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

			//Accedemos a la cola para cambiar el flag de atendido dependiendo del tipo de atencion
			pthread_mutex_lock(&mutexColaPacientes);
			if(atencionPaciente == 0 || atencionPaciente == 1){
				int reaccionRandom = calculaRandom(1, 100);
				if(reaccionRandom <= 10){
				    paciente->atendido = 4;
				}else{
				    paciente->atendido = 3;
				}
			}else{
				paciente->atendido = 6;
			}
			//Liberamos el mutex de la cola
			pthread_mutex_unlock(&mutexColaPacientes);
		}else{
			//Si el paciente es uno que reacciono
			sleep(5);

			pthread_mutex_lock(&mutexColaPacientes);
			paciente->atendido = 7;
			pthread_mutex_unlock(&mutexColaPacientes);
		}
		//Escribe en el fichero que termina la atencion
		sprintf(mensaje, "Termina la atencion al paciente nº%d", paciente->id);
        pthread_mutex_lock(&mutexFichero);
        writeLogMessage("Medico", mensaje);
        pthread_mutex_unlock(&mutexFichero);
    }	
}


/*
 * Hilo que representa al Enfermrer@
 * 
 * 
 * FIXME: Los enfermeros de otros tipos cogen pacientes que no deberian
 */
void *hiloEnfermero(void *arg) {
    //TODO Añadir a calcular reaccion
    char motivo[100];
    char mensaje[100];
    int duerme;
    int grupoVacunacion =* (int*) arg;  
    int i;
    struct Paciente *sigPaciente;


    while(1) {
        switch(grupoVacunacion) { //Sabiendo el grupo al que  vacuna buscara en un sitio u otro
            case 0: 
                pthread_mutex_lock(&mutexColaPacientes); //Bloqueamos lista para acceder al mutex
                sigPaciente = primerPaciente;
        		pthread_mutex_unlock(&mutexColaPacientes); //Como ya hemos atendido al paciente desbloqueamos la cola

		        i = 0;
                
                
                while(sigPaciente != NULL) {

		            pthread_mutex_lock(&mutexColaPacientes);
                    if (enfermero1.atendiendo == 0)
                    {
                        printf("Enfermero no esta atendiendo\n");
                    }
                    if (sigPaciente->tipo == 0)
                    {
                        printf("Sigpaciente tipo =0\n");
                    }
                    if (sigPaciente->atendido == 0)
                    {
                        printf("SsigPaciente->atendido == 0\n");
                    }
                    
                    

                    if(enfermero1.atendiendo == 0 && sigPaciente->tipo == 0 && sigPaciente->atendido == 0) {  //Comprobamos si hay del mismo tipo, si ha sido atendido y si ese enfermero esta atendiendo
        		        pthread_mutex_unlock(&mutexColaPacientes); //Como ya hemos atendido al paciente desbloqueamos la cola

                        enfermero1.atendiendo = 1;
                        enfermero1.pacientesAtendidos++;

                        int aleatorio = calculaRandom(0, 100);
                        pthread_mutex_lock(&mutexColaPacientes); //Bloqueamos lista para acceder al mutex

                        if(aleatorio < 80) {
                            duerme = calculaRandom(1, 4);
                            sprintf(motivo, "Motivo por el que fue atendido: El paciente %d tiene todo en regla", sigPaciente->id);
                            sigPaciente->atendido = 2;
                        }else if(aleatorio < 90) {
                            duerme = calculaRandom(2, 6);
                            sprintf(motivo, "Motivo por el que fue atendido: El paciente %d esta mal documentado", sigPaciente->id);
                            sigPaciente->atendido = 3;
                        }else {
                            duerme = calculaRandom(6, 10);
                            sprintf(motivo, "Motivo por el que NO fue atendido:El paciente %d tiene gripe", sigPaciente->id);
                            sigPaciente->atendido = 6;
                        }
        		        pthread_mutex_unlock(&mutexColaPacientes); //Como ya hemos atendido al paciente desbloqueamos la cola

                        pthread_mutex_lock(&mutexFichero);
                        sprintf(mensaje, "Comienza la atencion al paciente nº %d", sigPaciente->id);
                        writeLogMessage("Enfermero1", mensaje);
                        pthread_mutex_unlock(&mutexFichero);

                        printf("waw\n");
                        sleep(duerme);

                        pthread_mutex_lock(&mutexFichero);
                        if (aleatorio<90)
                        {
                            sprintf(mensaje, "Termina la atencion al paciente nº %d", sigPaciente->id);
                            writeLogMessage("Enfermero1", mensaje);
                        }
                        writeLogMessage("Enfermero1", motivo);
                        enfermero1.atendiendo=0;
                        
                        pthread_mutex_unlock(&mutexFichero);
                        

                        if(enfermero1.pacientesAtendidos == 5) { //Si es 5 entonces podra descansar
                            enfermero1.atendiendo = 0;//
                            enfermero1.pacientesAtendidos = 0; //Resetemaos el contador de pacientes para que pueda volver a empezar

                            sleep(5); //Descansa sus 5 segundos 

                            pthread_mutex_lock(&mutexFichero);
                            writeLogMessage("Enfermero1", "Enfermer@_1 esta descansando");
                            pthread_mutex_unlock(&mutexFichero);
                            //Aqui creo que habra que indicar a otro enfermero o al medico que debe vacunar
                        }
                        pthread_mutex_lock(&mutexColaPacientes); //Bloqueamos lista para acceder al mutex
                        sigPaciente->atendido = 1;//Marcamos el paciente como atendido
        		        pthread_mutex_unlock(&mutexColaPacientes); //Como ya hemos atendido al paciente desbloqueamos la cola

                    }
                    pthread_mutex_lock(&mutexColaPacientes); //Bloqueamos lista para acceder al mutex
                    sigPaciente = sigPaciente->sig;
        		    pthread_mutex_unlock(&mutexColaPacientes); //Como ya hemos atendido al paciente desbloqueamos la cola

                    i++;
                }


		        pthread_mutex_lock(&mutexColaPacientes);
                i = 0;
                sigPaciente = primerPaciente;
		        pthread_mutex_unlock(&mutexColaPacientes); //Como ya hemos atendido al paciente desbloqueamos la cola


                if(enfermero2.atendiendo == 0 || enfermero3.atendiendo == 0) {
                //No hay pacientes de tipo1, buscamos de otros tipos
                    while(i < contadorPacientes && sigPaciente != NULL) {
                        if(enfermero1.atendiendo == 0 && sigPaciente->atendido == 0) {  
                            enfermero1.atendiendo = 1;
                            enfermero1.pacientesAtendidos++;

                            int aleatorio = calculaRandom(0, 100);
		                    pthread_mutex_lock(&mutexColaPacientes);

                            if(aleatorio < 80) {
                                duerme = calculaRandom(1, 4);
                                sprintf(motivo, "El paciente %d tiene todo en regla", sigPaciente->id);
                                sigPaciente->atendido = 2;
                            }else if(aleatorio < 90) {
                                duerme = calculaRandom(2, 6);
                                sprintf(motivo, "El paciente %d esta mal documentado", sigPaciente->id);
                                sigPaciente->atendido = 3;
                            }else {
                                duerme = calculaRandom(6, 10);
                                sprintf(motivo, "El paciente %d tiene gripe", sigPaciente->id);
                                sigPaciente->atendido = 6;
                            }
        		            pthread_mutex_unlock(&mutexColaPacientes); //Como ya hemos atendido al paciente desbloqueamos la cola
                        
                            pthread_mutex_lock(&mutexFichero);
                            sprintf(mensaje, "Comienza la atencion al paciente nº %d", sigPaciente->id);
                            writeLogMessage("Enfermero1", mensaje);
                            pthread_mutex_unlock(&mutexFichero);

                            sleep(duerme);
                            
                            pthread_mutex_lock(&mutexFichero);
                            sprintf(mensaje, "Termina la atencion al paciente nº %d", sigPaciente->id);
                            writeLogMessage("Enfermero1",mensaje);
                            writeLogMessage("Enfermero1", motivo);
                            pthread_mutex_unlock(&mutexFichero);
                            enfermero1.atendiendo = 0;

                            if(enfermero1.pacientesAtendidos == 5) { //Si es 5 entonces podra descansar
                                enfermero1.atendiendo = 0;//ESTO SE PODRIA QUITAR YA QUE CUANDO LLEGA AL IF ES SI O SI 0
                                enfermero1.pacientesAtendidos = 0; //Resetemaos el contador de pacientes para que pueda volver a empezar
                                sleep(5); //Descansa sus 5 segundos 
                                pthread_mutex_lock(&mutexFichero);
                                writeLogMessage("Enfermero1", "Enfermer@_1 esta descansando");
                                pthread_mutex_unlock(&mutexFichero);
                            }
		                    pthread_mutex_lock(&mutexColaPacientes);
                            sigPaciente->atendido = 1;//Marcamos el paciente como atendido
		                    pthread_mutex_unlock(&mutexColaPacientes);

                        }
		                pthread_mutex_lock(&mutexColaPacientes);
                        sigPaciente = sigPaciente->sig;
                        pthread_mutex_unlock(&mutexColaPacientes);

                        i++;
                    }
                }
                //No ha encontrados pacientes, entonces libera mutex y duerme un sec para volver a empezar a buscar
                sleep(1);

                //break;
             case 1:
                i = 0;

                pthread_mutex_lock(&mutexColaPacientes); //Bloqueamos lista para acceder al mutex
                sigPaciente = primerPaciente;
                pthread_mutex_unlock(&mutexColaPacientes);


                while(sigPaciente != NULL) {  
                    printf("repeticion: i:%d", i);

                    if(enfermero2.atendiendo == 0 && sigPaciente->tipo == 1 && sigPaciente->atendido == 0) {  //Comprobamos si hay del mismo tipo, si ha sido atendido y si ese enfermero esta atendiendo
                        enfermero2.atendiendo = 1;
                        enfermero2.pacientesAtendidos++;

                        int aleatorio = calculaRandom(0, 100);
                        pthread_mutex_lock(&mutexColaPacientes); 
                        if(aleatorio < 80) {
                            duerme = calculaRandom(1, 4);
                            sprintf(motivo, "Motivo por el que fue atendido: El paciente %d tiene todo en regla", sigPaciente->id);
                            sigPaciente->atendido = 2;
                        }else if(aleatorio < 90) {
                            duerme = calculaRandom(2, 6);
                            sprintf(motivo, "Motivo por el que fue atendido: El paciente %d esta mal documentado", sigPaciente->id);
                            sigPaciente->atendido = 3;
                        }else {
                            duerme = calculaRandom(6, 10);
                            sprintf(motivo, "Motivo por el que NO fue atendido:El paciente %d tiene gripe", sigPaciente->id);
                            sigPaciente->atendido = 6;
                        }
                        pthread_mutex_unlock(&mutexColaPacientes);

                        sprintf(mensaje, "Comienza la atencion al paciente nº %d", sigPaciente->id);
                        pthread_mutex_lock(&mutexFichero);
                        writeLogMessage("Enfermero2", mensaje);
                        pthread_mutex_unlock(&mutexFichero);

                        sleep(duerme);
                        
                        pthread_mutex_lock(&mutexFichero);
                        sprintf(mensaje, "Termina la atencion al paciente nº %d\n",sigPaciente->id);
                        writeLogMessage("Enfermero2", mensaje);
                        writeLogMessage("Enfermero2", motivo);
                        pthread_mutex_unlock(&mutexFichero);
                        enfermero2.atendiendo = 0;

                        if(enfermero2.pacientesAtendidos == 5) { //Si es 5 entonces podra descansar
                            enfermero2.atendiendo = 0;//ESTE SE PODRIA QUITAR
                            enfermero2.pacientesAtendidos = 0; //Resetemaos el contador de pacientes para que pueda volver a empezar
                            sleep(5); //Descansa sus 5 segundos 
                            pthread_mutex_lock(&mutexFichero);
                            writeLogMessage("Enfermero2", "Enfermero esta descansando");
                            pthread_mutex_unlock(&mutexFichero);
                            //Aqui creo que habra que indicar a otro enfermero o al medico que debe vacunar
                        }

		                pthread_mutex_lock(&mutexColaPacientes);

                        sigPaciente->atendido = 1;//Marcamos el paciente como atendido
		                pthread_mutex_unlock(&mutexColaPacientes);

                    }
                    i++;
		            pthread_mutex_lock(&mutexColaPacientes);
                    sigPaciente = sigPaciente->sig;
		            pthread_mutex_unlock(&mutexColaPacientes);

                }



		        pthread_mutex_lock(&mutexColaPacientes);
		        i=0;
		        sigPaciente = primerPaciente;
		        pthread_mutex_unlock(&mutexColaPacientes); //Como ya hemos atendido al paciente desbloqueamos la cola

                //No hay pacientes de tipo1, buscamos de otros tipos
                if(enfermero1.atendiendo == 0 || enfermero3.atendiendo == 0) {
                    while(i < contadorPacientes && sigPaciente != NULL) {

                        if(enfermero2.atendiendo == 0 && sigPaciente->atendido == 0) {  
                            enfermero2.atendiendo = 1;
                            enfermero2.pacientesAtendidos++;

                            int aleatorio = calculaRandom(0, 100);

		                    pthread_mutex_lock(&mutexColaPacientes);
                            if(aleatorio < 80) {
                                duerme = calculaRandom(1, 4);
                                sprintf(motivo, "Motivo por el que fue atendido: El paciente %d tiene todo en regla", sigPaciente->id);
                                sigPaciente->atendido = 2;
                            }else if(aleatorio < 90) {
                                duerme = calculaRandom(2, 6);
                                sprintf(motivo, "Motivo por el que fue atendido: El paciente %d esta mal documentado", sigPaciente->id);
                                sigPaciente->atendido = 3;
                            }else {
                                duerme = calculaRandom(6, 10);
                                sprintf(motivo, "Motivo por el que NO fue atendido: El paciente %d tiene gripe", sigPaciente->id);
                                sigPaciente->atendido = 6;
                            }
		                    pthread_mutex_unlock(&mutexColaPacientes);


                            sprintf(mensaje, "Comienza la atencion al paciente nº %d", sigPaciente->id);
                            pthread_mutex_lock(&mutexFichero);
                            writeLogMessage("Enfermero2", mensaje);
                            pthread_mutex_unlock(&mutexFichero);

                            sleep(duerme);
                            
                            pthread_mutex_lock(&mutexFichero);
                            sprintf(mensaje, "Termina la atencion al paciente nº %d", sigPaciente->id);
                            writeLogMessage("Enfermero2", mensaje);
                            writeLogMessage("Enfermero2", motivo);
                            pthread_mutex_unlock(&mutexFichero);
                            enfermero.atendiendo = 0;

                            if(enfermero2.pacientesAtendidos == 5) { //Si es 5 entonces podra descansar
                                enfermero2.atendiendo = 0;//
                                enfermero2.pacientesAtendidos = 0; //Resetemaos el contador de pacientes para que pueda volver a empezar
                                sleep(5); //Descansa sus 5 segundos 
                                pthread_mutex_lock(&mutexFichero);
                                writeLogMessage("Enfermero2", "Enfermer@_2 esta descansando");
                                pthread_mutex_unlock(&mutexFichero);
                                //Aqui creo que habra que indicar a otro enfermero o al medico que debe vacunar
                            }
		                    pthread_mutex_lock(&mutexColaPacientes);
                            sigPaciente->atendido = 1;//Marcamos el paciente como atendido
		                    pthread_mutex_unlock(&mutexColaPacientes);

                        }
		                pthread_mutex_lock(&mutexColaPacientes);

                        sigPaciente = sigPaciente->sig;
		                pthread_mutex_unlock(&mutexColaPacientes);

                        i++;
                    }
                }

                //No ha encontrados pacientes, entonces libera mutex y duerme un sec para volver a empezar a buscar
                sleep(1);

                break;

            default:
                i = 0;

                pthread_mutex_lock(&mutexColaPacientes); //Bloqueamos lista para acceder al mutex
                sigPaciente = primerPaciente;
                pthread_mutex_unlock(&mutexColaPacientes);

                while(sigPaciente != NULL) {
		            pthread_mutex_lock(&mutexColaPacientes);

                    if(enfermero3.atendiendo == 0 && sigPaciente->tipo == 2 && sigPaciente->atendido == 0) {  //Comprobamos si hay del mismo tipo, si ha sido atendido y si ese enfermero esta atendiendo
                        pthread_mutex_unlock(&mutexColaPacientes);

                        enfermero3.atendiendo = 1;
                        enfermero3.pacientesAtendidos++;

                        int aleatorio = calculaRandom(0, 100);
                        pthread_mutex_lock(&mutexColaPacientes);

                        if(aleatorio < 80) {
                            duerme = calculaRandom(1, 4);
                            sprintf(motivo, "Motivo por el que fue atendido: El paciente %d tiene todo en regla", sigPaciente->id);
                            sigPaciente->atendido = 2;
                        }else if(aleatorio < 90) {
                            duerme = calculaRandom(2, 6);
                            sprintf(motivo, "Motivo por el que fue atendido: El paciente %d esta mal documentado", sigPaciente->id);
                            sigPaciente->atendido = 3;
                        }else {
                            duerme = calculaRandom(6, 10);
                            sprintf(motivo, "Motivo por el que fue atendido: El paciente %d tiene gripe", sigPaciente->id);
                            sigPaciente->atendido = 6;
                        }
                        pthread_mutex_unlock(&mutexColaPacientes);


                        pthread_mutex_lock(&mutexFichero);
                        sprintf(mensaje, "Comienza la atencion al paciente %d", sigPaciente->id);
                        writeLogMessage("Enfermero3", mensaje);
                        pthread_mutex_unlock(&mutexFichero);

                        sleep(duerme);
                        
                        pthread_mutex_lock(&mutexFichero);
                        sprintf(mensaje, "Termina la atencion al paciente %d", sigPaciente->id);
                        writeLogMessage("Enfermero3", mensaje);
                        writeLogMessage("Enfermero3", motivo);
                        pthread_mutex_unlock(&mutexFichero);
                        enfermero3.atendiendo = 0;

                        if(enfermero3.pacientesAtendidos == 5) { //Si es 5 entonces podra descansar
                            enfermero3.atendiendo = 0;//SE PODRIA QUITAR
                            enfermero3.pacientesAtendidos = 0; //Resetemaos el contador de pacientes para que pueda volver a empezar
                            sleep(5); //Descansa sus 5 segundos
                            pthread_mutex_lock(&mutexFichero);
                            writeLogMessage("Enfermero3", "Enfermer@_3 esta descansando");
                            pthread_mutex_unlock(&mutexFichero);
                            //Aqui creo que habra que indicar a otro enfermero o al medico que debe vacunar
                        }
		                pthread_mutex_lock(&mutexColaPacientes);
                        sigPaciente->atendido = 1;//Marcamos el paciente como atendido
                        pthread_mutex_unlock(&mutexColaPacientes);

                    }
                    i++;
		            pthread_mutex_lock(&mutexColaPacientes);
                    sigPaciente = sigPaciente->sig;
                    pthread_mutex_unlock(&mutexColaPacientes);

                }

		        pthread_mutex_lock(&mutexColaPacientes);
                i = 0;
                sigPaciente = primerPaciente;
		        pthread_mutex_unlock(&mutexColaPacientes); //Como ya hemos atendido al paciente desbloquamos la cola

                //No hay pacientes de tipo1, buscamos de otros tipos
                if(enfermero1.atendiendo == 0 || enfermero2.atendiendo == 0) {
                    while(i < contadorPacientes && sigPaciente != NULL) {
		                pthread_mutex_lock(&mutexColaPacientes);
                        if(enfermero3.atendiendo == 0 && sigPaciente->atendido == 0) {  
                            pthread_mutex_unlock(&mutexColaPacientes);
                            enfermero3.atendiendo = 1;
                            enfermero3.pacientesAtendidos++;

                            int aleatorio = calculaRandom(0, 100);
		                    pthread_mutex_lock(&mutexColaPacientes);
                            if(aleatorio < 80) {
                                duerme = calculaRandom(1, 4);
                                sprintf(motivo, "Motivo por el que fue atendido: El paciente %d tiene todo en regla", sigPaciente->id);
                                sigPaciente->atendido = 2;
                            }else if(aleatorio < 90) {
                                duerme = calculaRandom(2, 6);
                                sprintf(motivo, "Motivo por el que fue atendido: El paciente %d esta mal documentado", sigPaciente->id);
                                sigPaciente->atendido = 3;
                            }else {
                                duerme = calculaRandom(6, 10);
                                sprintf(motivo, "Motivo por el que fue atendido: El paciente %d tiene gripe", sigPaciente->id);
                                sigPaciente->atendido = 6;
                            }
                            pthread_mutex_unlock(&mutexColaPacientes);

                            writeLogMessage("Enfermero3", motivo);
                            pthread_mutex_lock(&mutexFichero);
                            sprintf(mensaje, "Comienza la atencion al paciente nº %d", sigPaciente->id);
                            writeLogMessage("Enfermero3", mensaje);
                            pthread_mutex_unlock(&mutexFichero);

                            sleep(duerme);
                            
                            pthread_mutex_lock(&mutexFichero);
                            sprintf(mensaje, "Termina la atencion al paciente nº %d", sigPaciente->id);
                            writeLogMessage("Enfermero3", mensaje);
                            pthread_mutex_unlock(&mutexFichero);
                            enfermero3.atendiendo = 0;

                            if(enfermero3.pacientesAtendidos == 5) { //Si es 5 entonces podra descansar
                                enfermero3.atendiendo = 0;//
                                enfermero3.pacientesAtendidos = 0; //Resetemaos el contador de pacientes para que pueda volver a empezar
                                sleep(5); //Descansa sus 5 segundos 
                                pthread_mutex_lock(&mutexFichero);
                                writeLogMessage("Enfermero3", "Enfermer@_3 esta descansando");
                                pthread_mutex_unlock(&mutexFichero);
                                //Aqui creo que habra que indicar a otro enfermero o al medico que debe vacunar
                            }
		                    pthread_mutex_lock(&mutexColaPacientes);
                            sigPaciente->atendido = 1;//Marcamos el paciente como atendido
                            pthread_mutex_unlock(&mutexColaPacientes);
                        }
    		        }
                }    
                sleep(1);
                break;
             
        } 
    }
}

/*
 * Hilo que representa al Estadístico
 */
void *hiloEstadistico(void *arg){
	while(1) {
		pthread_mutex_lock(&mutexColaPacientes);
		pthread_cond_wait(&varEstadistico, &mutexColaPacientes);
		pthread_mutex_unlock(&mutexColaPacientes);

		pthread_mutex_lock(&mutexFichero);
		writeLogMessage("Estadistico", "Comienza la actividad");
		pthread_mutex_unlock(&mutexFichero);

		sleep(4);

		pthread_mutex_lock(&mutexFichero);
        writeLogMessage("Estadistico", "Termina la actividad");               
        pthread_mutex_unlock(&mutexFichero);

		pthread_cond_signal(&varPacientes);
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

void eliminarPaciente(struct Paciente **pacienteAEliminar){
	printf("Eliminar Paciente %d\n", (*pacienteAEliminar)->id);
	if((*pacienteAEliminar)->ant != NULL && (*pacienteAEliminar)->sig != NULL){
		(*pacienteAEliminar)->ant->sig=(*pacienteAEliminar)->sig;
		(*pacienteAEliminar)->sig->ant = (*pacienteAEliminar)->ant;
	}else if((*pacienteAEliminar)->ant != NULL){
		primerPaciente = (*pacienteAEliminar)->sig;
		(*pacienteAEliminar)->sig->ant = NULL;
	}else if((*pacienteAEliminar)->sig != NULL){
		ultimoPaciente = (*pacienteAEliminar)->ant;
		(*pacienteAEliminar)->ant->sig = NULL;
	}else{
		(*pacienteAEliminar) = NULL;
		primerPaciente = NULL;
		ultimoPaciente = NULL;
	}
	printf("Eliminado el paciente \n");
}
