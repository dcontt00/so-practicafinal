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
     *  8 - si no ha dado reaccion
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
    int atendiendo; //0 si esta libre, 1 si esta atendiendo a un paciente, 2 si esta durmiendo
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

    /*pthread_create (&threadEnfermero1, NULL, hiloEnfermero, (void *)&n1);
    pthread_create (&threadEnfermero2, NULL, hiloEnfermero, (void *)&n2);
    pthread_create (&threadEnfermero3, NULL, hiloEnfermero, (void *)&n3);
*/
     //7. Crear el hilo médico.
    pthread_create (&medico, NULL, hiloMedico, NULL);
//8. Crear el hilo estadístico.
    //pthread_create (&estadistico, NULL, hiloEstadistico, NULL);
//9. Esperar por señales de forma infinita.
    while (1){
	    pause();
    }
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
        pthread_create (&threadNuevoPaciente, NULL, hiloPaciente, (void *)pacienteNuevo);
    }
    struct Paciente *aux;
    aux = primerPaciente;
    while(aux != NULL){
        printf("\nPaciente %d\n", aux->id);
        aux = aux->sig;
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
    //Se coge la referencia a paciente que se pasa por parametro
    paciente = (struct Paciente *) arg;
    printf("Tipo del paciente %d", paciente->tipo);
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

    sprintf(mensaje, "Entra el Paciente.");
    pthread_mutex_lock(&mutexFichero);
    writeLogMessage(type, mensaje);
    pthread_mutex_unlock(&mutexFichero);
    pthread_mutex_unlock(&mutexColaPacientes);
    do {
        sleep(3);
        pthread_mutex_lock(&mutexColaPacientes);
        atendido = paciente->atendido;
        comportamiento = calculaRandom(1, 10);
        printf("random(1)%d\n", comportamiento);
        if (atendido == 1) {
            printf("El paciente: %d esta siendo atentido", paciente->id);
        }else if(atendido == 0){
            printf("El paciente: %d no esta siendo atentido", paciente->id);
            if (comportamiento <= 3) {
                if(paciente->atendido == 0) {
                    if(comportamiento <= 2){
                        sprintf(mensaje, "Se ha cansado de esperar.");
                    }else{
                        sprintf(mensaje, "Se lo piensa mejor.");
                    }
                    pthread_mutex_lock(&mutexFichero);
                    writeLogMessage(type, mensaje);
                    pthread_mutex_unlock(&mutexFichero);
                    paciente->atendido = -1;
                }
            }else{
                comportamiento = calculaRandom(1, 100);
                printf("random(2)%d\n", comportamiento);

                // Va al baño y pierde el turno
                if (comportamiento <= 5) {
                    if(paciente->atendido == 0) {
                        sprintf(mensaje, "Se va al baño y pierde su turno.");
                        pthread_mutex_lock(&mutexFichero);
                        writeLogMessage(type, mensaje);
                        pthread_mutex_unlock(&mutexFichero);
                        paciente->atendido = -1; //TODO El paciente pierde el turno, no sale del consultorio
                    }


                } else {
                    sprintf(mensaje, "Decide esperar a su turno.");
                    pthread_mutex_lock(&mutexFichero);
                    writeLogMessage(type, mensaje);
                    pthread_mutex_unlock(&mutexFichero);

                }
            }
        }
        pthread_mutex_unlock(&mutexColaPacientes);
    }while (atendido == 0);

    while(atendido != -1) {
        //compruebo si el paciente tiene gripe.
        if (atendido == 6) {
            sprintf(mensaje, "Tiene gripe.");
            pthread_mutex_lock(&mutexColaPacientes);
            paciente->atendido = -1;
            pthread_mutex_unlock(&mutexColaPacientes);
        } else if(atendido == 4 || atendido == 5){
            //comprueba si da reaccion si da reaccion a la vacuna.
            pthread_mutex_lock(&mutexColaPacientes);
            sprintf(mensaje, "Ha dado reaccion a la vacuna.");
            pthread_mutex_lock(&mutexFichero);
            writeLogMessage(type, mensaje);
            pthread_mutex_unlock(&mutexFichero);
            pthread_mutex_unlock(&mutexColaPacientes);
            while(atendido == 4 || atendido == 5){
                pthread_mutex_lock(&mutexColaPacientes);
                atendido = paciente->atendido;
                pthread_mutex_unlock(&mutexColaPacientes);
                sleep(1);
            }
            pthread_mutex_lock(&mutexColaPacientes);
            paciente->atendido = -1;
            pthread_mutex_unlock(&mutexColaPacientes);
        }else if(atendido == 8){
            sprintf(mensaje, "No ha dado reaccion a la vacuna.");
            pthread_mutex_lock(&mutexFichero);
            writeLogMessage(type, mensaje);
            pthread_mutex_unlock(&mutexFichero);

            pthread_mutex_lock(&mutexColaPacientes);
            paciente->atendido = 7;
            pthread_mutex_unlock(&mutexColaPacientes);
        }

        if(atendido == 7){
            comportamiento = calculaRandom(1, 100);
            if (comportamiento <= 25) {
                sprintf(mensaje, "Decide participar en la prueba serologica.");
                pthread_mutex_lock(&mutexFichero);
                writeLogMessage(type, mensaje);
                pthread_mutex_unlock(&mutexFichero);
                pthread_mutex_lock(&mutexColaPacientes);
                paciente->serologia = 1;
                pthread_mutex_unlock(&mutexColaPacientes);

                pthread_cond_signal(&varEstadistico);
                sprintf(mensaje, "Esta preparado para el estudio.");
                pthread_mutex_lock(&mutexFichero);
                writeLogMessage(type, mensaje);
                pthread_mutex_unlock(&mutexFichero);

                pthread_mutex_lock(&mutexColaPacientes);//FIXME: Crear un nuevo mutex para estadistico?
                pthread_cond_wait(&varPacientes, &mutexColaPacientes);
                pthread_mutex_unlock(&mutexColaPacientes);
                sprintf(mensaje, "Abandona el estudio.");
                pthread_mutex_lock(&mutexFichero);
                writeLogMessage(type, mensaje);
                pthread_mutex_unlock(&mutexFichero);

                pthread_mutex_lock(&mutexColaPacientes);
                paciente->atendido = -1;
                pthread_mutex_unlock(&mutexColaPacientes);
            } else {
                sprintf(mensaje, "No va a participar en la prueba serologica.");
                pthread_mutex_lock(&mutexFichero);
                writeLogMessage(type, mensaje);
                pthread_mutex_unlock(&mutexFichero);
                pthread_mutex_lock(&mutexColaPacientes);
                paciente->atendido = -1;
                pthread_mutex_unlock(&mutexColaPacientes);
            }
        }
        pthread_mutex_lock(&mutexColaPacientes);
        atendido = paciente->atendido;
        pthread_mutex_unlock(&mutexColaPacientes);
    }
    sprintf(mensaje, "Abandona el consultorio");
    pthread_mutex_lock(&mutexFichero);
    writeLogMessage(type, mensaje);
 	pthread_mutex_unlock(&mutexFichero);


	eliminarPaciente(&paciente);
 	pthread_exit(NULL);

  }



/*
 * Hilo que representa al médico
 */
void *hiloMedico(void *arg){

	//Variable que guarda al paciente
	struct Paciente *paciente;
    struct Paciente *sigPaciente;
	int contador;
	int pacientesNoAtendidos;
	/*0 - si no es un paciente con reaccion
         *1 - si es un paciente con reaccion
         */
	int reaccion;
	//Se ejecuta indefinidamente hasta que se recibe el la señal
	while(1){
		paciente = NULL;
		while(paciente == NULL){
            sleep(1);
            pthread_mutex_lock(&mutexColaPacientes);
            contador = contadorPacientes;
            pthread_mutex_unlock(&mutexColaPacientes);
            pacientesNoAtendidos = 0;
            if(contador > 0){
				//Como accedemos a la lista bloqueamos el mutex
				pthread_mutex_lock(&mutexColaPacientes);
				/*
				 * Buscamos el paciente con reaccion que mas tiempo lleve esperando
				 * Si no hay paciente seguira siendo NULL y si hay pasa a ser la
				 * posicion del paciente.
				 */

                sigPaciente = primerPaciente;
				while(sigPaciente != NULL && paciente == NULL){

					if(sigPaciente->atendido == 4){
						paciente = sigPaciente;
						reaccion = 1;
					}
					sigPaciente = sigPaciente->sig;

				}


				sigPaciente = primerPaciente;
				while(sigPaciente != NULL){
                    if(sigPaciente->atendido == 0){
                        pacientesNoAtendidos++;
                    }
                    sigPaciente = sigPaciente->sig;
				}
				//Si no encuentra un paciente con reaccion y existen pacientes que no estan siendo atendidos
				if(paciente == NULL && pacientesNoAtendidos > 0){
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

					int i = -1;

                    sigPaciente = primerPaciente;
					while(sigPaciente != NULL){
					    i++;
						if(sigPaciente->atendido == 0){
							nPacientesTipo[sigPaciente->tipo] += 1;
							pacientesAntiguos[sigPaciente->tipo] = i;
						}
						sigPaciente = sigPaciente->sig;
					}
					//Si se ha encontrado algun paciente antiguo se le asigna a paciente el paciente mas antiguo
					if(pacientesAntiguos[1] != -1 || pacientesAntiguos[2] != -1 || pacientesAntiguos[0] != -1){
						if(nPacientesTipo[0] >= nPacientesTipo[1] && nPacientesTipo[0] >= nPacientesTipo[2]){
							int i = pacientesAntiguos[0];
                            sigPaciente = primerPaciente;
							while(i > 0){
								sigPaciente = sigPaciente->sig;
								i--;
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
				}else if(paciente != NULL){
					//Se le cambia el flag de atendido al paciente si tiene reaccion
					reaccion = 1;
					paciente->atendido = 5;
				}
				//Liberamos el mutex una vez terminamos las operaciones con la cola de pacientes
				pthread_mutex_unlock(&mutexColaPacientes);
				//Se espera 1 segundo para repetir el bucle si posPaciente es -1

			}
		}

		//Medico sale del bucle de buscar pacientes
		char mensaje[100];
		if(reaccion == 0){
            sprintf(mensaje, "Comienza la atencion al paciente nº%d", paciente->id);
            //Escribe en el fichero que comienza la atencion
            pthread_mutex_lock(&mutexFichero);
            writeLogMessage("Medico", mensaje);
            pthread_mutex_unlock(&mutexFichero);
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
				    paciente->atendido = 8;
				}
			}else{
				paciente->atendido = 6;
			}
            //Escribe en el fichero que termina la atencion
            sprintf(mensaje, "Termina de vacunar al paciente nº%d", paciente->id);
            pthread_mutex_lock(&mutexFichero);
            writeLogMessage("Medico", mensaje);
            pthread_mutex_unlock(&mutexFichero);
			//Liberamos el mutex de la cola
			pthread_mutex_unlock(&mutexColaPacientes);
		}else{
            //Si el paciente es uno que reacciono
            sprintf(mensaje, "Comienza la atencion al paciente nº%d con reaccion", paciente->id);
            //Escribe en el fichero que comienza la atencion
            pthread_mutex_lock(&mutexFichero);
            writeLogMessage("Medico", mensaje);
            pthread_mutex_unlock(&mutexFichero);

			sleep(5);

			pthread_mutex_lock(&mutexColaPacientes);
            //Escribe en el fichero que termina la atencion
            sprintf(mensaje, "Termina la atencion al paciente nº%d con reaccion", paciente->id);
            pthread_mutex_lock(&mutexFichero);
            writeLogMessage("Medico", mensaje);
            pthread_mutex_unlock(&mutexFichero);
			paciente->atendido = 7;
			pthread_mutex_unlock(&mutexColaPacientes);
		}

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
    //TODO Añadir mutex para acceder al estado de enfermero
    char motivo[100];
    char mensaje[100];
    int duerme;
    int grupoVacunacion =* (int*) arg;
    int atencion, reaccion;
    struct Paciente *sigPaciente;
    struct Paciente *paciente;


    while(1) {
        switch(grupoVacunacion) { //Sabiendo el grupo al que  vacuna buscara en un sitio u otro
            case 0:
                paciente = NULL;
                //Buscamos al Paciente
                while (paciente == NULL) {
                    sleep(1);
                    pthread_mutex_lock(&mutexColaPacientes); //Bloqueamos lista para acceder a la cola
                    //Buscamos a un paciente del mismo tipo
                    sigPaciente = primerPaciente;

                    while (sigPaciente != NULL && paciente == NULL) {
                        if (sigPaciente->tipo == 0 && sigPaciente->atendido == 0) {  //Comprobamos si hay del mismo tipo, si ha sido atendido y si ese enfermero esta atendiendo
                            paciente = sigPaciente;
                            enfermero1.atendiendo = 1;
                            enfermero1.pacientesAtendidos++;
                        }
                        sigPaciente = sigPaciente->sig;
                    }

                    //Si no se ha encontrado paciente se buscan pacientes de distintos tipos

                    if(paciente == NULL){
                        sigPaciente = primerPaciente;
                        //Si alguno de los otros enfermeros esta descansando
                        //se buscan los pacientes de ese tipo
                        if(enfermero2.atendiendo == 2){
                            while (sigPaciente != NULL && paciente == NULL) {
                                if (sigPaciente->tipo == 1 && sigPaciente->atendido == 0) {
                                    paciente = sigPaciente;
                                    enfermero1.atendiendo = 1;
                                    enfermero1.pacientesAtendidos++;
                                }
                                sigPaciente = sigPaciente->sig;
                            }
                        }else if(enfermero3.atendiendo == 2){
                            while (sigPaciente != NULL && paciente == NULL) {
                                if (sigPaciente->tipo == 2 && sigPaciente->atendido == 0) {
                                    paciente = sigPaciente;
                                    enfermero1.atendiendo = 1;
                                    enfermero1.pacientesAtendidos++;
                                }
                                sigPaciente = sigPaciente->sig;
                            }
                        }else{
                            if(enfermero2.atendiendo == 1 && enfermero3.atendiendo == 1) {
                                while (sigPaciente != NULL && paciente == NULL) {
                                    if (sigPaciente->atendido == 0) {
                                        paciente = sigPaciente;
                                        enfermero1.atendiendo = 1;
                                        enfermero1.pacientesAtendidos++;
                                    }
                                    sigPaciente = sigPaciente->sig;
                                }
                            }
                        }
                    }



                    if(paciente != NULL){
                        paciente->atendido = 1;
                    }

                    pthread_mutex_unlock(&mutexColaPacientes);
                }

                atencion = calcularAtencion();
                if(atencion == 0) {
                    duerme = calculaRandom(1, 4);
                    sprintf(motivo, "Motivo por el que fue atendido: El paciente %d tiene todo en regla", paciente->id);
                }else if(atencion == 1) {
                    duerme = calculaRandom(2, 6);
                    sprintf(motivo, "Motivo por el que fue atendido: El paciente %d esta mal documentado", paciente->id);
                }else {
                    duerme = calculaRandom(6, 10);
                    sprintf(motivo, "Motivo por el que no fue atendido:El paciente %d tiene gripe", paciente->id);
                }

                pthread_mutex_lock(&mutexFichero);
                sprintf(mensaje, "Comienza la atencion al paciente nº%d", paciente->id);
                writeLogMessage("Enfermer@_1", mensaje);
                pthread_mutex_unlock(&mutexFichero);

                sleep(duerme);

                pthread_mutex_lock(&mutexColaPacientes);
                if(atencion == 0 || atencion == 1) {
                    //Se calcula si le da reaccion o no
                    reaccion = calculaRandom(1, 100);
                    if(reaccion <= 10) {
                        paciente->atendido = 4;
                    }else{
                        paciente->atendido = 8;
                    }
                }else {
                    paciente->atendido = 6;
                }
                pthread_mutex_unlock(&mutexColaPacientes);

                pthread_mutex_lock(&mutexFichero);
                sprintf(mensaje, "Termina la atencion al paciente nº%d", paciente->id);
                writeLogMessage("Enfermer@_1", mensaje);
                writeLogMessage("Enfermer@_1", motivo);
                pthread_mutex_unlock(&mutexFichero);

                if(enfermero1.pacientesAtendidos == 5) { //Si es 5 entonces podra descansar
                    enfermero1.pacientesAtendidos = 0; //Resetemaos el contador de pacientes para que pueda volver a empezar
                    //Como otros hilos acceden a este atributo utilizamos mutex para cambiarlo
                    pthread_mutex_lock(&mutexColaPacientes);
                    enfermero1.atendiendo=2;
                    pthread_mutex_unlock(&mutexColaPacientes);
                    pthread_mutex_lock(&mutexFichero);
                    writeLogMessage("Enfermer@_1", "Esta descansando");
                    pthread_mutex_unlock(&mutexFichero);
                    sleep(5); //Descansa sus 5 segundos
                    pthread_mutex_lock(&mutexFichero);
                    writeLogMessage("Enfermer@_1", "Ha terminado de descansar");
                    pthread_mutex_unlock(&mutexFichero);
                }

                pthread_mutex_lock(&mutexColaPacientes);
                enfermero1.atendiendo = 0;
                pthread_mutex_unlock(&mutexColaPacientes);
                break;
            case 1:
                paciente = NULL;
                //Buscamos al Paciente
                while (paciente == NULL) {
                    sleep(1);
                    pthread_mutex_lock(&mutexColaPacientes); //Bloqueamos lista para acceder a la cola
                    //Buscamos a un paciente del mismo tipo
                    sigPaciente = primerPaciente;
                    while (sigPaciente != NULL && paciente == NULL) {
                        if (sigPaciente->tipo == 1 && sigPaciente->atendido == 0) {  //Comprobamos si hay del mismo tipo, si ha sido atendido y si ese enfermero esta atendiendo
                            paciente = sigPaciente;
                            enfermero2.atendiendo = 1;
                            enfermero2.pacientesAtendidos++;
                        }
                        sigPaciente = sigPaciente->sig;
                    }
                    //Si no se ha encontrado paciente se buscan pacientes de distintos tipos
                    if(paciente == NULL){
                        sigPaciente = primerPaciente;
                        //Si alguno de los otros enfermeros esta descansando
                        //se buscan los pacientes de ese tipo
                        if(enfermero1.atendiendo == 2){
                            while (sigPaciente != NULL && paciente == NULL) {
                                if (sigPaciente->tipo == 0 && sigPaciente->atendido == 0) {
                                    paciente = sigPaciente;
                                    enfermero2.atendiendo = 1;
                                    enfermero2.pacientesAtendidos++;
                                }
                                sigPaciente = sigPaciente->sig;
                            }
                        }else if(enfermero3.atendiendo == 2){
                            while (sigPaciente != NULL && paciente == NULL) {
                                if (sigPaciente->tipo == 2 && sigPaciente->atendido == 0) {
                                    paciente = sigPaciente;
                                    enfermero2.atendiendo = 1;
                                    enfermero2.pacientesAtendidos++;
                                }
                                sigPaciente = sigPaciente->sig;
                            }
                        }else{
                            if(enfermero1.atendiendo == 1 && enfermero3.atendiendo == 1) {
                                while (sigPaciente != NULL && paciente == NULL) {
                                    if (sigPaciente->atendido == 0) {
                                        paciente = sigPaciente;
                                        enfermero2.atendiendo = 1;
                                        enfermero2.pacientesAtendidos++;
                                    }
                                    sigPaciente = sigPaciente->sig;
                                }
                            }
                        }
                    }

                    if(paciente != NULL){
                        paciente->atendido = 1;
                    }
                    pthread_mutex_unlock(&mutexColaPacientes);
                }

                atencion = calcularAtencion();
                if(atencion == 0) {
                    duerme = calculaRandom(1, 4);
                    sprintf(motivo, "Motivo por el que fue atendido: El paciente %d tiene todo en regla", paciente->id);
                }else if(atencion == 1) {
                    duerme = calculaRandom(2, 6);
                    sprintf(motivo, "Motivo por el que fue atendido: El paciente %d esta mal documentado", paciente->id);
                }else {
                    duerme = calculaRandom(6, 10);
                    sprintf(motivo, "Motivo por el que no fue atendido:El paciente %d tiene gripe", paciente->id);
                }

                pthread_mutex_lock(&mutexFichero);
                sprintf(mensaje, "Comienza la atencion al paciente nº%d", paciente->id);
                writeLogMessage("Enfermer@_2", mensaje);
                pthread_mutex_unlock(&mutexFichero);

                sleep(duerme);

                pthread_mutex_lock(&mutexColaPacientes);
                if(atencion == 0 || atencion == 1) {
                    //Se calcula si le da reaccion o no
                    reaccion = calculaRandom(1, 100);
                    if(reaccion <= 10) {
                        paciente->atendido = 4;
                    }else{
                        paciente->atendido = 8;
                    }
                }else {
                    paciente->atendido = 6;
                }
                pthread_mutex_unlock(&mutexColaPacientes);

                pthread_mutex_lock(&mutexFichero);
                sprintf(mensaje, "Termina la atencion al paciente nº%d", paciente->id);
                writeLogMessage("Enfermer@_2", mensaje);
                writeLogMessage("Enfermer@_2", motivo);
                pthread_mutex_unlock(&mutexFichero);

                if(enfermero2.pacientesAtendidos == 5) { //Si es 5 entonces podra descansar
                    enfermero2.pacientesAtendidos = 0; //Resetemaos el contador de pacientes para que pueda volver a empezar
                    //Como otros hilos acceden a este atributo utilizamos mutex para cambiarlo
                    pthread_mutex_lock(&mutexColaPacientes);
                    enfermero2.atendiendo=2;
                    pthread_mutex_unlock(&mutexColaPacientes);
                    pthread_mutex_lock(&mutexFichero);
                    writeLogMessage("Enfermer@_2", "Esta descansando");
                    pthread_mutex_unlock(&mutexFichero);
                    sleep(5); //Descansa sus 5 segundos
                    pthread_mutex_lock(&mutexFichero);
                    writeLogMessage("Enfermer@_2", "Ha terminado de descansar");
                    pthread_mutex_unlock(&mutexFichero);
                }
                pthread_mutex_lock(&mutexColaPacientes);
                enfermero2.atendiendo = 0;
                pthread_mutex_unlock(&mutexColaPacientes);
                break;
            default:
                paciente = NULL;
                //Buscamos al Paciente
                while (paciente == NULL) {
                    sleep(1);
                    pthread_mutex_lock(&mutexColaPacientes); //Bloqueamos lista para acceder a la cola
                    //Buscamos a un paciente del mismo tipo
                    sigPaciente = primerPaciente;
                    while (sigPaciente != NULL && paciente == NULL) {
                        if (sigPaciente->tipo == 2 && sigPaciente->atendido == 0) {  //Comprobamos si hay del mismo tipo, si ha sido atendido y si ese enfermero esta atendiendo
                            paciente = sigPaciente;
                            enfermero3.atendiendo = 1;
                            enfermero3.pacientesAtendidos++;
                        }
                        sigPaciente = sigPaciente->sig;
                    }
                    //Si no se ha encontrado paciente se buscan pacientes de distintos tipos
                    if(paciente == NULL){
                        sigPaciente = primerPaciente;
                        //Si alguno de los otros enfermeros esta descansando
                        //se buscan los pacientes de ese tipo
                        if(enfermero1.atendiendo == 2){
                            while (sigPaciente != NULL && paciente == NULL) {
                                if (sigPaciente->tipo == 0 && sigPaciente->atendido == 0) {
                                    paciente = sigPaciente;
                                    enfermero3.atendiendo = 1;
                                    enfermero3.pacientesAtendidos++;
                                }
                                sigPaciente = sigPaciente->sig;
                            }
                        }else if(enfermero2.atendiendo == 2){
                            while (sigPaciente != NULL && paciente == NULL) {
                                if (sigPaciente->tipo == 1 && sigPaciente->atendido == 0) {
                                    paciente = sigPaciente;
                                    enfermero3.atendiendo = 1;
                                    enfermero3.pacientesAtendidos++;
                                }
                                sigPaciente = sigPaciente->sig;
                            }
                        }else{
                            if(enfermero1.atendiendo == 1 && enfermero2.atendiendo == 1) {
                                while (sigPaciente != NULL && paciente == NULL) {
                                    if (sigPaciente->atendido == 0) {
                                        paciente = sigPaciente;
                                        enfermero3.atendiendo = 1;
                                        enfermero3.pacientesAtendidos++;
                                    }
                                    sigPaciente = sigPaciente->sig;
                                }
                            }
                        }
                    }

                    if(paciente != NULL){
                        paciente->atendido = 1;
                    }
                    pthread_mutex_unlock(&mutexColaPacientes);
                }

                atencion = calcularAtencion();
                if(atencion == 0) {
                    duerme = calculaRandom(1, 4);
                    sprintf(motivo, "Motivo por el que fue atendido: El paciente %d tiene todo en regla", paciente->id);
                }else if(atencion == 1) {
                    duerme = calculaRandom(2, 6);
                    sprintf(motivo, "Motivo por el que fue atendido: El paciente %d esta mal documentado", paciente->id);
                }else {
                    duerme = calculaRandom(6, 10);
                    sprintf(motivo, "Motivo por el que no fue atendido:El paciente %d tiene gripe", paciente->id);
                }

                pthread_mutex_lock(&mutexFichero);
                sprintf(mensaje, "Comienza la atencion al paciente nº%d", paciente->id);
                writeLogMessage("Enfermer@_3", mensaje);
                pthread_mutex_unlock(&mutexFichero);

                sleep(duerme);

                pthread_mutex_lock(&mutexColaPacientes);
                if(atencion == 0 || atencion == 1) {
                    //Se calcula si le da reaccion o no
                    reaccion = calculaRandom(1, 100);
                    if(reaccion <= 10) {
                        paciente->atendido = 4;
                    }else{
                        paciente->atendido = 8;
                    }
                }else {
                    paciente->atendido = 6;
                }
                pthread_mutex_unlock(&mutexColaPacientes);

                pthread_mutex_lock(&mutexFichero);
                sprintf(mensaje, "Termina la atencion al paciente nº%d", paciente->id);
                writeLogMessage("Enfermer@_3", mensaje);
                writeLogMessage("Enfermer@_3", motivo);
                pthread_mutex_unlock(&mutexFichero);

                if(enfermero3.pacientesAtendidos == 5) { //Si es 5 entonces podra descansar
                    enfermero3.pacientesAtendidos = 0; //Resetemaos el contador de pacientes para que pueda volver a empezar
                    //Como otros hilos acceden a este atributo utilizamos mutex para cambiarlo
                    pthread_mutex_lock(&mutexColaPacientes);
                    enfermero3.atendiendo=2;
                    pthread_mutex_unlock(&mutexColaPacientes);
                    pthread_mutex_lock(&mutexFichero);
                    writeLogMessage("Enfermer@_3", "Esta descansando");
                    pthread_mutex_unlock(&mutexFichero);
                    sleep(5); //Descansa sus 5 segundos
                    pthread_mutex_lock(&mutexFichero);
                    writeLogMessage("Enfermer@_3", "Ha terminado de descansar");
                    pthread_mutex_unlock(&mutexFichero);
                }
                pthread_mutex_lock(&mutexColaPacientes);
                enfermero3.atendiendo = 0;
                pthread_mutex_unlock(&mutexColaPacientes);
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
    srand (time(NULL)*getpid());
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
    pthread_mutex_lock(&mutexColaPacientes);
    if(*pacienteAEliminar == NULL){
        printf("NULL\n");
    }
	printf("Eliminar Paciente %d\n", (*pacienteAEliminar)->id);

	if((*pacienteAEliminar)->ant != NULL && (*pacienteAEliminar)->sig != NULL){
	    printf("Opcion 1\n");
		(*pacienteAEliminar)->ant->sig=(*pacienteAEliminar)->sig;
		(*pacienteAEliminar)->sig->ant = (*pacienteAEliminar)->ant;
	}else if((*pacienteAEliminar)->ant != NULL){
        printf("Opcion 3\n");
        ultimoPaciente = (*pacienteAEliminar)->ant;
        (*pacienteAEliminar)->ant->sig = NULL;

	}else if((*pacienteAEliminar)->sig != NULL){
        printf("Opcion 2\n");
        primerPaciente = (*pacienteAEliminar)->sig;
        (*pacienteAEliminar)->sig->ant = NULL;
	}else{
	    printf("Opcion por defecto\n");
		primerPaciente = NULL;
		ultimoPaciente = NULL;
	}
    contadorPacientes --;
	free(*pacienteAEliminar);
	printf("Eliminado el paciente \n");
    pthread_mutex_unlock(&mutexColaPacientes);
}
