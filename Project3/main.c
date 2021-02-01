/*
// Projeto SO - exercicio 1, version 03
// Sistemas Operativos, DEI/IST/ULisboa 2017-18
// GRUPO 47 - João Freitas (87671) e Pedro Soares (87693)
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include "matrix2d.h"



/*--------------------------------------------------------------------
| Types
---------------------------------------------------------------------*/

typedef struct barreira_t{
  pthread_mutex_t    mutex;
  pthread_cond_t     esperar_por_todos;
  int                iteracao;
  int                numTrabsEmEspera;
  int                numTrabsTotais;
  int                acabou;
} Barreira_t;

typedef struct {
  int N;
  int id;
  int iter;
  DoubleMatrix2D *matrixptr, *matrix_auxptr;
} thread_info;


/*--------------------------------------------------------------------
| Variáveis globais
---------------------------------------------------------------------*/

DoubleMatrix2D *matrix, *matrix_aux;
int tam_fatia;
int trab;
Barreira_t *barreira;
double maxD;
int numDesviosMaxD;
/*--------------------------------------------------------------------
| Function: InitBarreira
---------------------------------------------------------------------*/
Barreira_t* InitBarreira(){
  Barreira_t *barreira = (Barreira_t*) malloc (sizeof(Barreira_t));

  if (barreira == NULL) {
    fprintf(stderr, "\nErro ao alocar memória para barreira\n");
    return NULL;
  }

  if(pthread_mutex_init(&(barreira->mutex), NULL) != 0) {
    fprintf(stderr, "\nErro ao inicializar mutex\n");
    exit(1);
  }


  if(pthread_cond_init(&(barreira->esperar_por_todos), NULL) != 0){
    fprintf(stderr, "\nErro ao inicializar variável de condição\n");
    exit(1);

  }

  barreira->numTrabsTotais = trab;
  barreira->numTrabsEmEspera = 0;
  barreira->acabou = 0;

  return barreira;
}

/*--------------------------------------------------------------------
| Function: DestroyBarreira
---------------------------------------------------------------------*/
void DestroyBarreira(Barreira_t *barreira){
  

  if(pthread_cond_destroy(&barreira->esperar_por_todos) != 0) {
    fprintf(stderr, "\nErro ao destruir variável de condição\n");
    exit(1);
  }

  if(pthread_mutex_destroy(&barreira->mutex) != 0) {
    fprintf(stderr, "\nErro ao destruir mutex\n");
    exit(1);
  }

  free (barreira);
}


/*--------------------------------------------------------------------
| Function: WaitBarreira
---------------------------------------------------------------------*/
void WaitBarreira(thread_info *tinfo, int iteracao){
  DoubleMatrix2D *tmp;

  if(pthread_mutex_lock(&barreira->mutex) != 0) {
    fprintf(stderr, "\nErro ao bloquear mutex\n");
    exit(EXIT_FAILURE);
  }

  barreira->numTrabsEmEspera++;


  if (barreira->numTrabsEmEspera == barreira->numTrabsTotais) {

    /* Nenhum ponto da matriz com desvio maior que maxD -> acaba de iterar */
    if(numDesviosMaxD==0) {

      barreira->acabou = 1;

      if(pthread_cond_broadcast(&barreira->esperar_por_todos) != 0) {
        fprintf(stderr, "\nErro ao desbloquear variável de condição\n");
        exit(EXIT_FAILURE);
      }
      
    }

    /* Existem pontos com desvio maior que maxD -> continua a iterar */
    else {
    numDesviosMaxD=0;
    barreira->numTrabsEmEspera = 0;
    barreira->iteracao++;

    tmp = matrix;
    matrix = matrix_aux;
    matrix_aux = tmp;
    
    
    if(pthread_cond_broadcast(&barreira->esperar_por_todos) != 0) {
      fprintf(stderr, "\nErro ao desbloquear variável de condição\n");
      exit(EXIT_FAILURE);
      }
      
    }
  }

  else {
    while(iteracao == barreira->iteracao && barreira->numTrabsEmEspera < barreira->numTrabsTotais){
      
      if (pthread_cond_wait(&barreira->esperar_por_todos, &barreira->mutex) != 0) {
        fprintf(stderr, "\nErro ao esperar pela variável de condição\n");
        exit(EXIT_FAILURE);
      }
    }
  }

  if(pthread_mutex_unlock(&barreira->mutex) != 0) {
    fprintf(stderr, "\nErro ao desbloquear mutex\n");
    exit(EXIT_FAILURE);
  }
  
}


/*--------------------------------------------------------------------
| Function: tarefa_trabalhadora
---------------------------------------------------------------------*/

void *tarefa_trabalhadora (void *args) {
  
  thread_info *tinfo = (thread_info *) args;
  int iter;
  int i, j;

  
  /* Ciclo Iterativo */
  for (iter = 0; iter < tinfo->iter; iter++) {

    if(barreira->acabou == 1){
      break;
    }

    /* Calcular Pontos Internos */
    for (i = (tinfo->id-1) * tam_fatia ; i < (tinfo->id) * tam_fatia; i++) {
      for (j = 0; j < tinfo->N; j++) {
        double val = (dm2dGetEntry(matrix, i, j+1) +
                      dm2dGetEntry(matrix, i+2, j+1) +
                      dm2dGetEntry(matrix, i+1, j) +
                      dm2dGetEntry(matrix, i+1, j+2))/4;
        dm2dSetEntry(matrix_aux, i+1, j+1, val);
        
        /* contar pontos em que ainda há desvio maior que maxD */
        if(val-dm2dGetEntry(matrix, i+1, j+1)>maxD || val-dm2dGetEntry(matrix, i+1, j+1)<-maxD) {
          numDesviosMaxD++;
        }
      }
    }
    
    WaitBarreira(tinfo, iter);
  }

  pthread_exit(NULL);
  
}



/*--------------------------------------------------------------------
| Function: parse_integer_or_exit
---------------------------------------------------------------------*/

int parse_integer_or_exit(char const *str, char const *name)
{
  int value;
 
  if(sscanf(str, "%d", &value) != 1) {
    fprintf(stderr, "\nErro no argumento \"%s\".\n\n", name);
    exit(1);
  }
  return value;
}

/*--------------------------------------------------------------------
| Function: parse_double_or_exit
---------------------------------------------------------------------*/

double parse_double_or_exit(char const *str, char const *name)
{
  double value;

  if(sscanf(str, "%lf", &value) != 1) {
    fprintf(stderr, "\nErro no argumento \"%s\".\n\n", name);
    exit(1);
  }
  return value;
}


/*--------------------------------------------------------------------
| Function: main
---------------------------------------------------------------------*/
int main (int argc, char** argv) {
  int res;
  int i;
  //int tam_fatia;
  thread_info *tinfo;
  pthread_t *trabalhadoras;

  if(argc != 9) {
    fprintf(stderr, "\nNumero invalido de argumentos.\n");
    fprintf(stderr, "Uso: heatSim N tEsq tSup tDir tInf iteracoes trab maxD\n\n");
    return 1;
  }

  /* argv[0] = program name */
  int N = parse_integer_or_exit(argv[1], "N");
  double tEsq = parse_double_or_exit(argv[2], "tEsq");
  double tSup = parse_double_or_exit(argv[3], "tSup");
  double tDir = parse_double_or_exit(argv[4], "tDir");
  double tInf = parse_double_or_exit(argv[5], "tInf");
  int iter = parse_integer_or_exit(argv[6], "iteracoes");
  trab = parse_integer_or_exit(argv[7], "trab");
  maxD = parse_double_or_exit(argv[8], "maxD");



  fprintf(stderr, "\nArgumentos:\n"
  " N=%d tEsq=%.1f tSup=%.1f tDir=%.1f tInf=%.1f iter=%d trab=%d maxD=%.1f\n",
  N, tEsq, tSup, tDir, tInf, iter, trab, maxD);

  /* Verificações de Input */
  if(N < 1 || tEsq < 0 || tSup < 0 || tDir < 0 || tInf < 0 || iter < 1 || trab < 1 || maxD < 0) {
    fprintf(stderr, "\nErro: Argumentos invalidos.\n"
  " Lembrar que N >= 1, temperaturas >= 0, iter >= 1, trab >=1 e maxD >= 0\n\n");
    return -1;
  }

  if (N % trab != 0) {
    fprintf(stderr, "\nErro: Argumento %s e %s invalidos\n"
            "%s deve ser multiplo de %s.", "N", "trab", "N", "trab");
    return -1;
  }


  /* Calcular Tamanho de cada Fatia */
  tam_fatia = N/trab;

  /* Iniciar barreira */
  barreira = InitBarreira();

  /* Criar matrix e matrix_aux */
  matrix = dm2dNew(N+2, N+2);
  matrix_aux = dm2dNew(N+2, N+2);


  if (matrix == NULL || matrix_aux == NULL) {
    fprintf(stderr, "\nErro: Nao foi possivel alocar memoria para as matrizes.\n\n");
    return -1;
  }

  for(i=0; i<N+2; i++)
  dm2dSetLineTo(matrix, i, 0);

  dm2dSetLineTo (matrix, 0, tSup);
  dm2dSetLineTo (matrix, N+1, tInf);
  dm2dSetColumnTo (matrix, 0, tEsq);
  dm2dSetColumnTo (matrix, N+1, tDir);

  dm2dCopy (matrix_aux, matrix);


  /* Reservar Memória para Trabalhadoras */
  tinfo = (thread_info *)malloc(trab * sizeof(thread_info));
  trabalhadoras = (pthread_t *)malloc(trab * sizeof(pthread_t));

  if (tinfo == NULL || trabalhadoras == NULL) {
    fprintf(stderr, "\nErro ao alocar memória para trabalhadoras.\n");    
    return -1;
  }

  /* Criar Trabalhadoras */
  for (i = 0; i < trab; i++) {
    tinfo[i].N = N;
    tinfo[i].id = i + 1;
    tinfo[i].iter = iter;
    tinfo[i].matrixptr = matrix;
    tinfo[i].matrix_auxptr = matrix_aux;
    res = pthread_create(&trabalhadoras[i], NULL, tarefa_trabalhadora, &tinfo[i]);

    if(res != 0) {
      fprintf(stderr, "\nErro ao criar uma tarefa trabalhadora.\n");
      return -1;
    }
  }


  /* Esperar que as Trabalhadoras Terminem */
  
  for (i = 0; i < trab; i++) {
    res = pthread_join(trabalhadoras[i], NULL);
    
    if (res != 0) {
      fprintf(stderr, "\nErro ao esperar por uma tarefa trabalhadora.\n");    
      return -1;
    }  
  }

  DestroyBarreira(barreira);

  dm2dPrint(matrix);
  dm2dFree(matrix);
  dm2dFree(matrix_aux);

  return 0;
}
