/*
// Projeto SO
// Sistemas Operativos, DEI/IST/ULisboa 2017-18
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "matrix2d.h"
#include "mplib4.h"

/*--------------------------------------------------------------------
| Type: thread_info
| Description: Estrutura com Informação para Trabalhadoras
---------------------------------------------------------------------*/

typedef struct { 
    int id;
    int N;
    int iter;
    int trab;
    int tam_fatia;
}thread_info;


/*--------------------------------------------------------------------
| Function: tarefa_trabalhadora
| Description: Função executada por cada tarefa trabalhadora.
|              Recebe como argumento uma estrutura do tipo thread_info.
---------------------------------------------------------------------*/

void *tarefa_trabalhadora(void* args) {
  thread_info *tinfo = (thread_info *) args;
  DoubleMatrix2D *fatias[2];
  int atual, prox;
  int iter;
  int res, tam_msg;
  int i, j;

  /* Criar Matrizes */
  fatias[0] = dm2dNew(tinfo->tam_fatia+2,tinfo->N+2);
  fatias[1] = dm2dNew(tinfo->tam_fatia+2,tinfo->N+2);
  
  if (fatias[0] == NULL || fatias[1] == NULL) {
    fprintf(stderr, "\nErro ao criar Matrix2d numa trabalhadora.\n"); 
    exit(-1);
  }

  /* Receber Fatia da Tarefa Mestre */
  tam_msg = (tinfo->N+2) * sizeof(double);
  for (i = 0; i < tinfo->tam_fatia + 2; i++){
    res = receberMensagem(0, tinfo->id, dm2dGetLine(fatias[0], i), tam_msg);
    if(res != tam_msg) {
      fprintf(stderr, "\nErro ao receber mensagem da tarefa mestre.\n"); 
      exit(-1);
    }
  }
  
  dm2dCopy(fatias[1], fatias[0]);

  /* Ciclo Iterativo */
  for (iter = 0; iter < tinfo->iter; iter++) {
    atual = iter % 2;
    prox = 1 - iter % 2;

    /* Calcular Pontos Internos */
    for (i = 0; i < tinfo->tam_fatia; i++) {
      for (j = 0; j < tinfo->N; j++) {
        double val = (dm2dGetEntry(fatias[atual], i, j+1) +
                      dm2dGetEntry(fatias[atual], i+2, j+1) +
                      dm2dGetEntry(fatias[atual], i+1, j) +
                      dm2dGetEntry(fatias[atual], i+1, j+2))/4;
        dm2dSetEntry(fatias[prox], i+1, j+1, val);
      }
    }
    
    /* Enviar e Receber Novos Valores das Linhas Adjacentes. As Tarefas Par Enviam antes de Receberem. */
    if(tinfo->id % 2 == 0) {
      if(tinfo->id > 1) {
        if(enviarMensagem(tinfo->id, tinfo->id - 1, dm2dGetLine(fatias[prox], 1), tam_msg) != tam_msg) {
            fprintf(stderr, "\nErro ao enviar mensagem entre trabalhadoras.\n"); 
            exit(-1);
        }
        if(receberMensagem(tinfo->id - 1, tinfo->id, dm2dGetLine(fatias[prox], 0), tam_msg) != tam_msg) {
            fprintf(stderr, "\nErro ao receber mensagem entre trabalhadoras.\n"); 
            exit(-1);
        }
      }
      if(tinfo->id < tinfo->trab) {
        if(enviarMensagem(tinfo->id, tinfo->id + 1, dm2dGetLine(fatias[prox], tinfo->tam_fatia), tam_msg) != tam_msg) {
            fprintf(stderr, "\nErro ao enviar mensagem entre trabalhadoras.\n"); 
            exit(-1);
        }
        if(receberMensagem(tinfo->id + 1, tinfo->id, dm2dGetLine(fatias[prox], tinfo->tam_fatia + 1), tam_msg) != tam_msg) {
            fprintf(stderr, "\nErro ao receber mensagem entre trabalhadoras.\n"); 
            exit(-1);
        }
      }
    }
    else {
      if(tinfo->id > 1) {
        if(receberMensagem(tinfo->id - 1, tinfo->id, dm2dGetLine(fatias[prox], 0), tam_msg) != tam_msg) {
            fprintf(stderr, "\nErro ao receber mensagem entre trabalhadoras.\n"); 
            exit(-1);
        }
        if(enviarMensagem(tinfo->id, tinfo->id - 1, dm2dGetLine(fatias[prox], 1), tam_msg) != tam_msg) {
            fprintf(stderr, "\nErro ao enviar mensagem entre trabalhadoras.\n"); 
            exit(-1);
        }
      }
      if(tinfo->id < tinfo->trab) {
        if(receberMensagem(tinfo->id + 1, tinfo->id, dm2dGetLine(fatias[prox], tinfo->tam_fatia + 1), tam_msg) != tam_msg) {
            fprintf(stderr, "\nErro ao receber mensagem entre trabalhadoras.\n"); 
            exit(-1);
        }
        if(enviarMensagem(tinfo->id, tinfo->id + 1, dm2dGetLine(fatias[prox], tinfo->tam_fatia), tam_msg) != tam_msg) {
            fprintf(stderr, "\nErro ao enviar mensagem entre trabalhadoras.\n"); 
            exit(-1);
        }
      }    
    }
  }
  
  /* Enviar Fatia Final à Tarefa Mestre */
  for (i = 0; i < tinfo->tam_fatia; i++) {
    res = enviarMensagem(tinfo->id, 0, dm2dGetLine(fatias[prox], i + 1), tam_msg);
    if(res != tam_msg) {
      fprintf(stderr, "\nErro ao enviar mensagem à tarefa mestre.\n"); 
      exit(-1);
    }
  }

  /* Libertar Memoria Alocada */
  dm2dFree(fatias[0]);
  dm2dFree(fatias[1]);

  pthread_exit(NULL);  
}

/*--------------------------------------------------------------------
| Function: parse_integer_or_exit
| Description: Processa a string str, do parâmetro name, como um inteiro
---------------------------------------------------------------------*/

int parse_integer_or_exit(char const *str, char const *name) {
  int value;
 
  if(sscanf(str, "%d", &value) != 1) {
    fprintf(stderr, "\nErro no argumento \"%s\".\n\n", name);
    exit(-1);
  }

  return value;
}

/*--------------------------------------------------------------------
| Function: parse_double_or_exit
| Description: Processa a string str, do parâmetro name, como um double
---------------------------------------------------------------------*/

double parse_double_or_exit(char const *str, char const *name) {
  double value;

  if(sscanf(str, "%lf", &value) != 1) {
    fprintf(stderr, "\nErro no argumento \"%s\".\n\n", name);
    exit(-1);
  }

  return value;
}

/*--------------------------------------------------------------------
| Function: main
| Description: Entrada do programa
---------------------------------------------------------------------*/

int main (int argc, char** argv) {
  int N;
  double tEsq, tSup, tDir, tInf;
  int iter;
  int trab;
  int csz;
  int tam_fatia;
  int res, tam_msg;
  int i, j;
  DoubleMatrix2D  *matrix;
  thread_info *tinfo;
  pthread_t *trabalhadoras;


 if(argc != 9) {
    fprintf(stderr, "\nNúmero de Argumentos Inválido.\n");
    fprintf(stderr, "Utilização: heatSim_p1 N tEsq tSup tDir tInf iter trab csz\n\n");
    return -1;
  }

  /* Ler Input */
  N = parse_integer_or_exit(argv[1], "n");
  tEsq = parse_double_or_exit(argv[2], "tEsq");
  tSup = parse_double_or_exit(argv[3], "tSup");
  tDir = parse_double_or_exit(argv[4], "tDir");
  tInf = parse_double_or_exit(argv[5], "tInf");
  iter = parse_integer_or_exit(argv[6], "iter");
  trab = parse_integer_or_exit(argv[7], "trab");
  csz =  parse_integer_or_exit(argv[8], "csz");

  fprintf(stderr, "\nArgumentos:\n"
   " N=%d tEsq=%.1f tSup=%.1f tDir=%.1f tInf=%.1f iter=%d trab=%d csz=%d",
   N, tEsq, tSup, tDir, tInf, iter, trab, csz);


  /* Verificações de Input */
  if(N < 1 || tEsq < 0 || tSup < 0 || tDir < 0 || tInf < 0 || iter < 1 || trab < 1 || csz < 0) {
    fprintf(stderr, "\nErro: Argumentos invalidos.\n"
  " Lembrar que N >= 1, temperaturas >= 0, iter >= 1, trab >=1 e csz >= 0\n\n");
    return -1;
  }

  if (N % trab != 0) {
    fprintf(stderr, "\nErro: Argumento %s e %s invalidos\n"
            "%s deve ser multiplo de %s.", "N", "trab", "N", "trab");
    return -1;
  }

  /* Inicializar Biblioteca de Passagem de Mensagens */
  if(inicializarMPlib(csz, trab + 1) != 0) {
    fprintf(stderr, "\nErro ao inicializar MPlib\n");
    return -1;
  }

  /* Calcular Tamanho de cada Fatia */
  tam_fatia = N/trab;

  /* Criar Matriz Inicial */
  matrix = dm2dNew(N+2, N+2);
  
  if (matrix == NULL) {
    fprintf(stderr, "\nErro ao criar Matrix2d.\n");    
    return -1;
  }
  
  dm2dSetLineTo(matrix, 0, tSup);
  dm2dSetLineTo(matrix, N+1, tInf);
  dm2dSetColumnTo(matrix, 0, tEsq);
  dm2dSetColumnTo(matrix, N+1, tDir);
  
  /* Reservar Memória para Trabalhadoras */
  tinfo = (thread_info *)malloc(trab * sizeof(thread_info));
  trabalhadoras = (pthread_t *)malloc(trab * sizeof(pthread_t));

  if (tinfo == NULL || trabalhadoras == NULL) {
    fprintf(stderr, "\nErro ao alocar memória para trabalhadoras.\n");    
    return -1;
  }

  /* Criar Trabalhadoras */
  for (i = 0; i < trab; i++) {
    tinfo[i].id = i+1;
    tinfo[i].N = N;
    tinfo[i].iter = iter;
    tinfo[i].trab = trab;
    tinfo[i].tam_fatia = tam_fatia;
    res = pthread_create(&trabalhadoras[i], NULL, tarefa_trabalhadora, &tinfo[i]);

    if(res != 0) {
      fprintf(stderr, "\nErro ao criar uma tarefa trabalhadora.\n");
      return -1;
    }
  }
  

  /* Enviar Fatias a cada Trabalhadora */
  tam_msg = (N+2) * sizeof(double);
  for (i = 0; i < trab; i++) {
    for (j = 0; j < (tam_fatia + 2); j++) {
      res = enviarMensagem(0, i+1, dm2dGetLine(matrix, i*tam_fatia + j), tam_msg);
      if(res != tam_msg) {
        fprintf(stderr, "\nErro ao enviar mensagem para trabalhadora.\n");
        return -1;
      }
    }
  }
  
  /* Receber Fatias Finais de cada Trabalhadora e Guardar na Matriz */
  for (i = 0; i < trab; i++) {
    for (j = 0; j < tam_fatia; j++) {
      res = receberMensagem(i+1, 0, dm2dGetLine(matrix, i*tam_fatia + j + 1), tam_msg);
      if(res != tam_msg) {
        fprintf(stderr, "\nErro ao receber mensagem de uma trabalhadora.\n");
        return -1;
      }
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
  
  /* Imprimir resultado */
  dm2dPrint(matrix);

  /* Libertar Memória */
  dm2dFree(matrix);
  free(tinfo);
  free(trabalhadoras);
  libertarMPlib();
 
  return 0;
}