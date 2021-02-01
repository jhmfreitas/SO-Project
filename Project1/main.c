/*
// Projeto SO - exercicio 1, version 03
// Sistemas Operativos, DEI/IST/ULisboa 2017-18
// GRUPO 47:
    -> 87671 - João Henrique Marques de Freitas
    -> 87693 - Pedro Amaral Soares
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

#include "matrix2d.h"
#include "mplib3.h"


// Argumentos para a tarefa 'fatia'
typedef struct args {
  int id;
  int nIteracoes;
  int nTrabalhadoras;
  int N;
} Args;

/*======================================================================
======================================================================*/


/*--------------------------------------------------------------------
| Function: simul_par
---------------------------------------------------------------------*/
void *simul_par(Args *arg, DoubleMatrix2D* fatia, DoubleMatrix2D* fatia_aux) {

  double value;
  int i, j, iter;
  double *buffer;
  
  DoubleMatrix2D *f, *f_aux, *tmp;

  int k = (arg->N) / (arg->nTrabalhadoras);

  buffer = (double*)malloc((arg->N + 2) * sizeof(double));


  f = fatia;
  f_aux = fatia_aux;

  dm2dCopy(f_aux,f);
  

  // calculo de temperaturas na fatia interior

  for (iter = 0; iter < arg->nIteracoes; iter++) {
    for (i = 1; i <= k; i++) {
      for (j = 1; j <= arg->N; j++) {
        value = (dm2dGetEntry(f, i-1, j) + dm2dGetEntry(f, i+1, j) +
          dm2dGetEntry(f, i, j-1) + dm2dGetEntry(f, i, j+1) ) / 4.0;

          dm2dSetEntry(f_aux, i, j, value);
      }
    }
  

    // trocas de temperaturas entre fatias adjacentes (sincronizacao das tarefas escravas)

    if (arg->id > 1) {
      enviarMensagem(arg->id, arg->id - 1, dm2dGetLine(f_aux,1), (arg->N + 2) *sizeof(double));
      receberMensagem(arg->id - 1, arg->id, buffer, (arg->N + 2) *sizeof(double));
      dm2dSetLine(f_aux, 0, buffer);
    }

    if (arg->id < arg->nTrabalhadoras) {
      receberMensagem(arg->id + 1, arg->id, buffer, (arg->N + 2) *sizeof(double));
      dm2dSetLine(f_aux, k+1, buffer);
      enviarMensagem(arg->id, arg->id + 1, dm2dGetLine(f_aux,k), (arg->N + 2) *sizeof(double));
    }


    // atualizacao da fatia
    tmp = f_aux;
    f_aux = f;
    f = tmp;
  }

  dm2dFree(f_aux);
  free(buffer);
  return f;
}


/*--------------------------------------------------------------------
| Thread: thread_fatia
---------------------------------------------------------------------*/
void *thread_fatia (void *arg) {

  Args *x =(Args*) arg;
  DoubleMatrix2D *fatia = dm2dNew (((x->N)/(x->nTrabalhadoras))+2, (x->N) + 2);
  DoubleMatrix2D *fatia_aux = dm2dNew (((x->N)/(x->nTrabalhadoras))+2, (x->N) + 2);
  
  double *buffer;
  buffer = (double*)malloc((x->N + 2) * sizeof(double));
  int k = x->N / x->nTrabalhadoras;


  // rececao da linhas da matriz correspondentes a esta fatia
  for(int i = 0; i <= k + 1; i++) {
    receberMensagem(0, x->id, buffer, (x->N+2) * sizeof(double));
    dm2dSetLine(fatia, i, buffer);
  }


  // preenchimento/cálculo da fatia
  fatia = simul_par(x,fatia, fatia_aux);

  if (fatia == NULL) {
    printf("\nErro na simulacao.\n\n");
    exit(1);
  }


  // envio das linhas da fatia para tarefa mestre
  for (int i = 1; i <= k; i++) {
    enviarMensagem(x->id, 0, dm2dGetLine(fatia, i), (x->N+2) * sizeof(double));
  }
  

  dm2dFree(fatia);
  free(buffer);
  
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

  if(argc != 9) {
    fprintf(stderr, "\nNumero invalido de argumentos.\n");
    fprintf(stderr, "Uso: heatSim N tEsq tSup tDir tInf iteracoes trabalhadoras mensagens\n\n");
    return 1;
  }

  /* argv[0] = program name */
  int N = parse_integer_or_exit(argv[1], "N");
  double tEsq = parse_double_or_exit(argv[2], "tEsq");
  double tSup = parse_double_or_exit(argv[3], "tSup");
  double tDir = parse_double_or_exit(argv[4], "tDir");
  double tInf = parse_double_or_exit(argv[5], "tInf");
  int iteracoes = parse_integer_or_exit(argv[6], "iteracoes");
  int trab = parse_integer_or_exit(argv[7], "trabalhadoras");
  int csz = parse_integer_or_exit(argv[8], "mensagens");

  DoubleMatrix2D *matrix;


  fprintf(stderr, "\nArgumentos:\n"
	" N=%d tEsq=%.1f tSup=%.1f tDir=%.1f tInf=%.1f iteracoes=%d trabalhadoras=%d mensagens=%d\n",
	N, tEsq, tSup, tDir, tInf, iteracoes, trab, csz);

  if(N < 1 || tEsq < 0 || tSup < 0 || tDir < 0 || tInf < 0 || iteracoes < 1 || trab <= 0 || (N % trab)!=0 || csz < 0) {
    fprintf(stderr, "\nErro: Argumentos invalidos.\n"
	" Lembrar que N >= 1, temperaturas >= 0 e iteracoes >= 1 e trab > 0 e mod(N/trab) == 0 e csz >= 0\n\n");
    return 1;
  }

  matrix = dm2dNew(N+2, N+2);

  if (matrix == NULL) {
    fprintf(stderr, "\nErro: Nao foi possivel alocar memoria para as matrizes.\n\n");
    return -1;
  }


  int i,j;

  for(i=0; i<N+2; i++)
    dm2dSetLineTo(matrix, i, 0);

  dm2dSetLineTo (matrix, 0, tSup);
  dm2dSetLineTo (matrix, N+1, tInf);
  dm2dSetColumnTo (matrix, 0, tEsq);
  dm2dSetColumnTo (matrix, N+1, tDir);


  /*-------------------------------------------------------------------------------------
  Preparacao de fatias e troca de mensagens com estas para, no fim, atualizarmos a matriz
  -------------------------------------------------------------------------------------*/

  int k = N/trab; // numero de linhas interiores numa fatia

  Args argumentos[trab];

  pthread_t tid[trab];

  if (inicializarMPlib(csz, (trab+1)) != 0) {
    fprintf(stderr, "\nErro ao alocar memória para a matriz.\n");
    return 1;
  }

  for(i = 0; i < trab; i++){
    argumentos[i].id = i+1;
    argumentos[i].nIteracoes = iteracoes;
    argumentos[i].N = N;
    argumentos[i].nTrabalhadoras = trab;

    if (pthread_create (&tid[i], NULL, thread_fatia, &argumentos[i]) != 0) {
      printf("Erro ao criar tarefa.\n");
      return 1;
    }
    
    // Enviar a cada tarefa/fatia as suas linhas correspondentes
    for(j = (k*i); j <= (k*(i + 1)) + 1; j++) {
      enviarMensagem(0, argumentos[i].id, dm2dGetLine(matrix, j), (N+2) * sizeof(double));
    }
    
  }


  double *bufferLine;
  bufferLine = (double*)malloc((N + 2) * sizeof(double));

  // receber os novos valores para a matriz
  for (i = 0; i < trab; i++) {

    for(j = (k*i) + 1; j <= (k*(i + 1)); j++) {
      receberMensagem(argumentos[i].id, 0, bufferLine, (N+2) * sizeof(double));
      dm2dSetLine(matrix, j, bufferLine);
    }
    
  }

  for (i = 0; i < trab; i++) {
    
    if (pthread_join (tid[i], NULL) != 0) {
      printf("Erro ao esperar por tarefa.\n");
      return 2;
    }
  }

  free(bufferLine);
  dm2dPrint(matrix);
  dm2dFree(matrix);
  libertarMPlib();
  }
