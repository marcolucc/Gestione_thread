/**
 * \author Lucchese Marco 
 * \mainpage Progetto IPC Sistemi Operativi - thread
 * \section Istruzioni
 * \subsection Step1 make
 * \subsection Step2 ./nome_file.x path_matriceA path_matriceB path_matriceC ordine 
 * <ul>
 * <li> path_matriceA: path del file contenente la matrice A <br>
 * <li> path_matriceB: path del file contenente la matrice A <br>
 * <li> path_matriceC: path del file contenente la matrice A <br>
 * <li> ordine: ordine delle matrici A e B quadrata <br>
 * </ul>
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/ipc.h>

#include <pthread.h>

pthread_mutex_t my_sync;


/// @brief funzione che spiega l'uso dei parametri nel programma
void usage(){
		char str[] = "help:\n Parametro 1. Il path del file contenente la matrice A. \n Parametro 2. Il path del file contenente la matrice B.\n Parametro 3. Il path del file in cui salvare la matrice C.\n Parametro 4. L’Ordine N (dimensione) delle due matrici.\n Parametro 5. Il numero P di processi concorrenti da utilizzare. \n";
		write(0,str, sizeof(str));
		exit(1);
}

/// @brief funzione richiamata nel caso il numero di parametri non sia uguale a 5
void errore_numero_parametri(){
	char str[] = "numero di parametri non corretto\n";
	write(0,str,sizeof(str));
	usage();
}
/// @brief funzione per la lettura delle matrici dai rispettivi file
void lettura_matrici(char *p,  int n, int mat[n][n]){
 
    int fd;																		/// file descriptor 
    int i,j,k;																	/// variabili di supporto 
    int nread;																	/// numero di byte letti  
    char c[512]={0};															/// char[] di scrittura
    
    if ((fd = open(p, O_RDONLY)) == -1)											/// apro il file 
        perror("lettura_matrice1");
    if((nread = read(fd, &c, 512)) == -1)										/// leggo il file
        perror("lettura_matrice1");
    /// inizializzo a 0 la metrice
    for(i=0;i<n;i++)
        for(j=0;j<n;j++)
            mat[i][j] = 0; 
    k=0;
    /// riempo con i valori da char a int 
    for(i=0;i<n;i++){
        for(j=0;j<n;j++){
            while(c[k]!=' ' && c[k]!='\n' && c[k]!='\0'){
                mat[i][j] = 10 * mat[i][j] + (c[k]-'0');
                k++;
            }
            k++;
        }
    }
    close(fd);																	/// chiudo il file descriptor
} 


/// definizione dell'argomento che viene passato alla funzione di startup delle thread
typedef struct {
    int     id;       /* id della thread */
    int     size; /* dimensione della matrice */
    int     Arow; /* indice della riga */
    int     Bcol; /* indice della colonna */
    int     *matA;
    int     *matB;
    int     *matC;
    int     *somma;
} package_t;


/// @brief funzione per la stampa su file
void stampa(int n, char *path, int *ptr_mem){
	int fd;																		/// file descriptor 
	if( (fd = open(path, O_WRONLY, 0777)) == -1){								/// apro il file 
		perror("OPEN FILE: ");
	}
	int *ptr;																	/// puntatore di supporto
	ptr = ptr_mem;
	int i;																		/// variabile di supporto 
	char buffer[10] = {'0'};													/// char[] temporaneo
	/// scrittura su file
	for(i = 0; i < n*n; i++){
		
		sprintf(buffer, "%d", *ptr);
		if( write(fd, &buffer, strlen(buffer)) == -1){
		perror("WRITE ERROR: ");	
		
		}
		if( write(fd, " ", sizeof(char)) == -1){
			perror("WRITE ERROR: ");	
		}
		if(((i+1) % n) == 0){
			if( write(fd,"\n", sizeof(char)) == -1){
				perror("WRITE ERROR: ");	
			}
		}
		ptr++;
	}
	close(fd);																	/// chiudo il file descriptor
}

void mult(int size, int row, int column, int *MA,int *MB,int *MC) {

    int position;
    *((MC + (row * size))+column) = 0;
    //MC[row][column] = 0;

    for(position = 0; position < size; position++) {
        *((MC + (row * size))+column) = *((MC + (row * size))+column) + (*((MA + (row * size))+position)  * *((MB + (position * size))+column) ) ;
    }
}

void sum(int size, int row, int column, int *MC, int *somma_elementi){
    printf("********sono nella funzione di somma, MC %i", *MC );
    int *ptr_c;
    ptr_c = MC + (row * size) + column;
    printf(", e ptr %i ", *ptr_c );
    
    pthread_mutex_lock(&my_sync);
    
    printf("  sono dentro   ");
    *somma_elementi += *ptr_c;
    
    pthread_mutex_unlock(&my_sync);
    
    printf(", e somma1 %i \n", *somma_elementi );
    
    
    
}

///funzione di startup per le threads
void *mult_worker(void *arg) {
    printf("sono nel worker di moltiplicazione \n");
    /* l'argomento della funzione deve essere castato al tipo desiderato */
    package_t *p = (package_t *)arg;

    printf("MATRIX THREAD %d: processing A row %d, B col %d\n", p->id, p->Arow, p->Bcol );

    /* chiamata alla funzione che esegue il prodotto riga * colonna */
    mult(p->size, p->Arow, p->Bcol, p->matA, p->matB, p->matC);

    printf("MATRIX THREAD %d: complete\n", p->id);

    /* liberato lo spazio allocato per p */
    free(p);

    return(NULL);

}
///funzione di startup per le threads
void *mult_worker_somma(void *arg) {
    printf("sono nel worker di somma \n");
    /* l'argomento della funzione deve essere castato al tipo desiderato */
    package_t *p = (package_t *)arg;

    printf("MATRIX THREAD %d: processing row %i column %i\n", p->id, p->Arow, p->Bcol );

    /* chiamata alla funzione che esegue il prodotto riga * colonna */
    sum(p->size, p->Arow, p->Bcol, p->matC, p->somma);

    printf("MATRIX THREAD %d: complete\n", p->id);

    /* liberato lo spazio allocato per p */
    free(p);

    return(NULL);

}


///  Il main alloca le matrici, assegna i loro valori e crea le thread per eseguire la moltiplicazione riga * colonna
int main(int argc, char *argv[]) {

    int size, row, column, num_threads, i;
    pthread_t *threads;                                                     /// per tenere traccia degli identificatori delle thread 

    package_t *p;                                                           /// argomento per le thread 
    
    size = *argv[4] - '0';
    int MA[size][size],MB[size][size],MC[size][size];
    int somma_elementi = 0;



/// viene creata una thread per ogni elemento della matrice,
    threads = (pthread_t *) malloc(size * size * sizeof(pthread_t));

    
     /// Inizializzazione dei valori delle matrici A e B.
     /// Questi valori potrebbero essere letti da file o
     /// ottenuti da altri processi.
     /// Dal momento che lo scopo principale dell'esempio
     /// � quello di mostrare il funzionamento delle thread
     /// non ci preoccupiamo di come vengono ottenuti
     /// i dati per le matrici A e B.
   
    
    /// Controllo se i parametri sono 5, verifico di non aver inserito --help */
	if (argc != 5) {
		if(argc > 1 && (strcmp("--help", argv[1]) == 0)) 
			usage();
		else
			errore_numero_parametri();
	}
    /// lettura della matrice 
	lettura_matrici(argv[1],size,MA);
	lettura_matrici(argv[2],size,MB);
	

    /// stampa dei valori degli elementi della matrice A
    printf("MATRIX MAIN THREAD: The A array is ;\n");
    for(row = 0; row < size; row ++) {
        for (column = 0; column < size; column++) {
            printf("%5d ",MA[row][column]);
        }
        printf("\n");
    }

    /// stampa dei valori degli elementi della matrice B 
    printf("MATRIX MAIN THREAD: The B array is ;\n");
    for(row = 0; row < size; row ++) {
        for (column = 0; column < size; column++) {
            printf("%5d ",MB[row][column]);
        }
        printf("\n");
    }
    // ****************************************************************************


    ///  Viene creato un thread per ogni elemento della matrice risultato C
 
    num_threads = 0;
    for(row = 0; row < size; row++) {
        for (column = 0; column < size; column++) {

            p = (package_t *)malloc(sizeof(package_t));
            p->id = num_threads;
            p->size = size;
            p->Arow = row;
            p->Bcol = column;
            p->matA = (int *)&MA;
            p->matB = (int *)&MB;
            p->matC = (int *)&MC;
            /*
              creazione della thread.
              il primo argomento  il puntatore all'id del thread.
              il secondo rappresenta gli attributi. NULL = attributi di default (joinable)
              il terzo � il puntatore alla funzione di startup
              il quarto � l'argomento per la funzione di startup.
              Vedere man pthread_create
            */
            pthread_create(&threads[num_threads], NULL, mult_worker, p);

            printf("MATRIX MAIN THREAD: thread %d created\n", num_threads);

            num_threads++;

        }
    }


    
    /// Sincronizzazione delle thread
    /*  Il main attende che tutte abbiano terminato la loro elaborazione
      usando la funzione pthread_join.
      vedere man pthread_join
    */
    for (i = 0; i < (size*size); i++) {
        pthread_join(threads[i], NULL);
        printf("MATRIX MAIN THREAD: child %d has joined\n", i);
    }


    /* Stampa dei risultati */
    printf("MATRIX MAIN THREAD: The resulting matrix C is;\n");
    for(row = 0; row < size; row ++) {
        for (column = 0; column < size; column++) {
            printf("%5d ",MC[row][column]);
        }
        printf("\n");
    }
    /// stampo su file 
    stampa(size, argv[3], (int *)MC);
    
    
    /// somma 
    ///  Viene creato un thread per ogni elemento della matrice risultato C
    pthread_mutex_init(&my_sync, NULL);
    num_threads = 0;
    for(row = 0; row < size; row++) {
        for (column = 0; column < size; column++) {

            p = (package_t *)malloc(sizeof(package_t));
            p->id = num_threads;
            p->size = size;
            p->Arow = row;
            p->Bcol = column;
            /// questa volta passo solo MC e somma
            p->matC = (int *)&MC;
            p->somma = &somma_elementi;
            /*
              creazione della thread.
              il primo argomento  il puntatore all'id del thread.
              il secondo rappresenta gli attributi. NULL = attributi di default (joinable)
              il terzo � il puntatore alla funzione di startup
              il quarto � l'argomento per la funzione di startup.
              Vedere man pthread_create
            */
            pthread_create(&threads[num_threads], NULL, mult_worker_somma, p);

            printf("MATRIX MAIN THREAD: thread %d created\n", num_threads);

            num_threads++;

        }
    }
    
    /// Sincronizzazione delle thread
    /* Il main attende che tutte abbiano terminato la loro elaborazione
       usando la funzione pthread_join.
       vedere man pthread_join
    */
    for (i = 0; i < (size*size); i++) {
        pthread_join(threads[i], NULL);
        printf("-2-MATRIX MAIN THREAD: child %d has joined\n", i);
    }
    
    printf(" ** SOMMA : %i **", somma_elementi);

    pthread_mutex_destroy(&my_sync);

    
    

    return 0;
}

