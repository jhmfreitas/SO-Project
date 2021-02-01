/*
// Projeto SO - exercicio 4
// Sistemas Operativos, DEI/IST/ULisboa 2017-18
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>

#include <string.h>

#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "matrix2d.h"
#include "util.h"



/*--------------------------------------------------------------------
| Type: thread_info
| Description: Estrutura com Informacao para Trabalhadoras
---------------------------------------------------------------------*/

typedef struct {
  int    id;
  int    N;
  int    iter;
  int    trab;
  int    tam_fatia;
  double maxD;
} thread_info;

/*--------------------------------------------------------------------
| Type: doubleBarrierWithMax
| Description: Barreira dupla com variavel de max-reduction
---------------------------------------------------------------------*/

typedef struct {
  int             total_nodes;
  int             pending[2];
  double          maxdelta[2];
  int             iteracoes_concluidas;
  pthread_mutex_t mutex;
  pthread_cond_t  wait[2];
} DualBarrierWithMax;



/*--------------------------------------------------------------------
| Global variables
---------------------------------------------------------------------*/

DoubleMatrix2D     *matrix_copies[2];
DualBarrierWithMax *dual_barrier;
double              maxD;

thread_info *tinfo;
pthread_t *trabalhadoras;

DoubleMatrix2D *matrixFromFile;

FILE *file;
int periodoS;
char *fichS;
int backup;
struct sigaction act;
int ctrl_c;



void handler (int s) {
  
  if(s == SIGALRM) {
    backup = 1;
    alarm(periodoS);
  }

  else if (s == SIGINT) {
    ctrl_c = 1;
    while (ctrl_c == 1);
    wait(NULL);
    exit(0);
  }
}


/*--------------------------------------------------------------------
| Function: dualBarrierInit
| Description: Inicializa uma barreira dupla
---------------------------------------------------------------------*/

DualBarrierWithMax *dualBarrierInit(int ntasks) {
  DualBarrierWithMax *b;
  b = (DualBarrierWithMax*) malloc (sizeof(DualBarrierWithMax));
  if (b == NULL) return NULL;

  b->total_nodes = ntasks;
  b->pending[0]  = ntasks;
  b->pending[1]  = ntasks;
  b->maxdelta[0] = 0;
  b->maxdelta[1] = 0;
  b->iteracoes_concluidas = 0;

  if (pthread_mutex_init(&(b->mutex), NULL) != 0) {
    fprintf(stderr, "\nErro a inicializar mutex\n");
    exit(1);
  }
  if (pthread_cond_init(&(b->wait[0]), NULL) != 0) {
    fprintf(stderr, "\nErro a inicializar variável de condição\n");
    exit(1);
  }
  if (pthread_cond_init(&(b->wait[1]), NULL) != 0) {
    fprintf(stderr, "\nErro a inicializar variável de condição\n");
    exit(1);
  }
  return b;
}

/*--------------------------------------------------------------------
| Function: dualBarrierFree
| Description: Liberta os recursos de uma barreira dupla
---------------------------------------------------------------------*/

void dualBarrierFree(DualBarrierWithMax* b) {

  if (pthread_mutex_destroy(&(b->mutex)) != 0) {
    fprintf(stderr, "\nErro a destruir mutex\n");
    exit(1);
  }
  if (pthread_cond_destroy(&(b->wait[0])) != 0) {
    fprintf(stderr, "\nErro a destruir variável de condição\n");
    exit(1);
  }
  if (pthread_cond_destroy(&(b->wait[1])) != 0) {
    fprintf(stderr, "\nErro a destruir variável de condição\n");
    exit(1);
  }
  free(b);
}
/*--------------------------------------------------------------------
| Function: dualBarrierWait
| Description: Ao chamar esta funcao, a tarefa fica bloqueada ate que
|              o numero 'ntasks' de tarefas necessario tenham chamado
|              esta funcao, especificado ao ininializar a barreira em
|              dualBarrierInit(ntasks). Esta funcao tambem calcula o
|              delta maximo entre todas as threads e devolve o
|              resultado no valor de retorno
---------------------------------------------------------------------*/

double dualBarrierWait (DualBarrierWithMax* b, int current, double localmax) {
  int next = 1 - current;

  if (pthread_mutex_lock(&(b->mutex)) != 0) {
    fprintf(stderr, "\nErro a bloquear mutex\n");
    exit(1);
  }
  // decrementar contador de tarefas restantes
  b->pending[current]--;
  // actualizar valor maxDelta entre todas as threads
  if (b->maxdelta[current]<localmax)
    b->maxdelta[current]=localmax;


  //====================================
  if (backup == 1 || ctrl_c == 1) {
    
    wait(NULL);

    int pid = fork();
    
      if (pid == 0) {
        
        // Criar/abrir ficheiro de salvaguarda
        pid = getpid();
        FILE *file_aux;
        char *fichS_aux = (char*)malloc(strlen(fichS)+2);
        if (fichS_aux == NULL) {
          die("Erro ao criar nome do ficheiro auxiliar");
        }
        
        strcpy(fichS_aux,fichS);
        fichS_aux = strcat(fichS_aux, "~");

        file_aux = fopen(fichS_aux, "w");
        if (file_aux == NULL ) {
           die("Erro ao abrir o ficheiro auxiliar");         
        }

        dm2dPrint (matrix_copies[current], file_aux);
        fclose(file_aux);
        
        if (rename(fichS_aux, fichS) == -1) {
          printf("FAIL\n");
        }

        ctrl_c = 0;
        exit(0);
      }

      else if (pid > 0) {
        waitpid(pid, NULL, 0);
        ctrl_c = 0;
        backup = 0;
      }

      else {
        printf("ERRO EM FORK\n");
      }
  }

  //====================================

  // verificar se sou a ultima tarefa
  if (b->pending[current]==0) {
    // sim -- inicializar proxima barreira e libertar threads
    b->iteracoes_concluidas++;
    b->pending[next]  = b->total_nodes;
    b->maxdelta[next] = 0;

    if (pthread_cond_broadcast(&(b->wait[current])) != 0) {
      fprintf(stderr, "\nErro a assinalar todos em variável de condição\n");
      exit(1);
    }
  }
  else {
    // nao -- esperar pelas outras tarefas
    while (b->pending[current]>0) 
    if (pthread_cond_wait(&(b->wait[current]), &(b->mutex)) != 0) {
      fprintf(stderr, "\nErro a esperar em variável de condição\n");
      exit(1);
    }
  }
  double maxdelta = b->maxdelta[current];
  if (pthread_mutex_unlock(&(b->mutex)) != 0) {
    fprintf(stderr, "\nErro a desbloquear mutex\n");
    exit(1);
  }
  return maxdelta;
}

/*--------------------------------------------------------------------
| Function: tarefa_trabalhadora
| Description: Funcao executada por cada tarefa trabalhadora.
|              Recebe como argumento uma estrutura do tipo thread_info
---------------------------------------------------------------------*/

void *tarefa_trabalhadora(void *args) {
  thread_info *tinfo = (thread_info *) args;
  int tam_fatia = tinfo->tam_fatia;
  int my_base = tinfo->id * tam_fatia;
  double global_delta = INFINITY;
  int iter = 0;

  do {
    int atual = iter % 2;
    int prox = 1 - iter % 2;
    double max_delta = 0;

    // Calcular Pontos Internos
    for (int i = my_base; i < my_base + tinfo->tam_fatia; i++) {
      for (int j = 0; j < tinfo->N; j++) {
        double val = (dm2dGetEntry(matrix_copies[atual], i,   j+1) +
                      dm2dGetEntry(matrix_copies[atual], i+2, j+1) +
                      dm2dGetEntry(matrix_copies[atual], i+1, j) +
                      dm2dGetEntry(matrix_copies[atual], i+1, j+2))/4;
        // calcular delta
        double delta = fabs(val - dm2dGetEntry(matrix_copies[atual], i+1, j+1));
        if (delta > max_delta) {
          max_delta = delta;
        }
        dm2dSetEntry(matrix_copies[prox], i+1, j+1, val);
      }
    }
    // barreira de sincronizacao; calcular delta global
    global_delta = dualBarrierWait(dual_barrier, atual, max_delta);
  } while (++iter < tinfo->iter && global_delta >= tinfo->maxD && ctrl_c == 0 );

  return 0;
}



/*--------------------------------------------------------------------
| Function: main
| Description: Entrada do programa
---------------------------------------------------------------------*/

int main (int argc, char** argv) {
  int N;
  double tEsq, tSup, tDir, tInf;
  int iter, trab;
  int tam_fatia;
  int res;
  int s;

  if (argc != 11) {
    fprintf(stderr, "Utilizacao: ./heatSim N tEsq tSup tDir tInf iter trab maxD fichS periodoS\n\n");
    die("Numero de argumentos invalido");
  }

  // Ler Input
  N    = parse_integer_or_exit(argv[1], "N",    1);
  tEsq = parse_double_or_exit (argv[2], "tEsq", 0);
  tSup = parse_double_or_exit (argv[3], "tSup", 0);
  tDir = parse_double_or_exit (argv[4], "tDir", 0);
  tInf = parse_double_or_exit (argv[5], "tInf", 0);
  iter = parse_integer_or_exit(argv[6], "iter", 1);
  trab = parse_integer_or_exit(argv[7], "trab", 1);
  maxD = parse_double_or_exit (argv[8], "maxD", 0);
  fichS = argv[9];
  periodoS = parse_double_or_exit (argv[10], "periodoS", 0);


  if (N % trab != 0) {
    fprintf(stderr, "\nErro: Argumento %s e %s invalidos.\n"
                    "%s deve ser multiplo de %s.", "N", "trab", "N", "trab");
    return -1;
  }


  // criar matrizes
  matrix_copies[0] = dm2dNew(N+2,N+2);
  matrix_copies[1] = dm2dNew(N+2,N+2);
  if (matrix_copies[0] == NULL || matrix_copies[1] == NULL) {
    die("Erro ao criar matrizes");
  }
  
  dm2dSetLineTo (matrix_copies[0], 0, tSup);
  dm2dSetLineTo (matrix_copies[0], N+1, tInf);
  dm2dSetColumnTo (matrix_copies[0], 0, tEsq);
  dm2dSetColumnTo (matrix_copies[0], N+1, tDir);
  dm2dCopy (matrix_copies[1],matrix_copies[0]);

  
  // Criar/abrir ficheiro de salvaguarda
  file = fopen(fichS, "r+");
  if (file == NULL ) {
    file = fopen(fichS,"wb");
    dm2dPrint(matrix_copies[0], file);  // base para a matriz de salvaguarda
  }

  else {
    matrixFromFile = dm2dNew(N+2,N+2);
    
    dm2dCopy(matrixFromFile,readMatrix2dFromFile(file, N+2, N+2));
    printf("N = %d", matrixFromFile->n_l);

    if (matrixFromFile == NULL) {
      file = fopen(fichS, "wb");
    }

    else {
      // recuperar matrizes
      dm2dCopy(matrix_copies[0],matrixFromFile);
      dm2dCopy(matrix_copies[1],matrixFromFile);
    }

    dm2dFree(matrixFromFile);
  }

  fclose(file);
  
  

  // Inicializar Barreira
  dual_barrier = dualBarrierInit(trab);
  if (dual_barrier == NULL)
    die("Nao foi possivel inicializar barreira");

  // Calcular tamanho de cada fatia
  tam_fatia = N / trab;


  // Reservar memoria para trabalhadoras
  tinfo = (thread_info*) malloc(trab * sizeof(thread_info));
  trabalhadoras = (pthread_t*) malloc(trab * sizeof(pthread_t));

  if (tinfo == NULL || trabalhadoras == NULL) {
    die("Erro ao alocar memoria para trabalhadoras");
  }

  //=======================
  // Inicializar backup
  if(sigemptyset(&act.sa_mask)==-1){
    die("Erro em sigaemptyset");
  }

  if(sigaddset(&act.sa_mask, SIGALRM)==-1){
    die("Erro em sigaddset");
  }

  if(sigaddset(&act.sa_mask, SIGINT)==-1){
    die("Erro em sigaddset");
  }

  s = pthread_sigmask(SIG_BLOCK, &act.sa_mask, NULL);
  if (s != 0)
    die("Erro com sigmask");


  // Criar trabalhadoras
  for (int i=0; i < trab; i++) {
    tinfo[i].id = i;
    tinfo[i].N = N;
    tinfo[i].iter = iter;
    tinfo[i].trab = trab;
    tinfo[i].tam_fatia = tam_fatia;
    tinfo[i].maxD = maxD;

    res = pthread_create(&trabalhadoras[i], NULL, tarefa_trabalhadora, &tinfo[i]);
    if (res != 0) {
      die("Erro ao criar uma tarefa trabalhadora");
    }

  }

  s = pthread_sigmask(SIG_UNBLOCK, &act.sa_mask, NULL);
  if (s != 0)
    die("Erro com sigmask");

  sigemptyset(&act.sa_mask);
  act.sa_handler = handler;

  if(sigaction(SIGALRM, &act, NULL)==-1){
    die("Erro em sigaction");
  }

  if(sigaction(SIGINT, &act, NULL)==-1){
    die("Erro em sigaction");
  }

  alarm(periodoS);

  

  // Esperar que as trabalhadoras terminem
  for (int i=0; i<trab; i++) {
    res = pthread_join(trabalhadoras[i], NULL);
    if (res != 0)
      die("Erro ao esperar por uma tarefa trabalhadora");
  }
  wait(NULL);
  

  dm2dPrint (matrix_copies[dual_barrier->iteracoes_concluidas%2], stdout);

  // Libertar memoria
  dm2dFree(matrix_copies[0]);
  dm2dFree(matrix_copies[1]);
  free(tinfo);
  free(trabalhadoras);
  dualBarrierFree(dual_barrier);
  unlink(fichS);
  
  return 0;
}