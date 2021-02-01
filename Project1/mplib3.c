/*
// Biblioteca de troca de mensagens, versao 3
// Sistemas Operativos, DEI/IST/ULisboa 2017-18
*/

#include "mplib3.h"
#include "leQueue.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


/*--------------------------------------------------------------------
| Types
---------------------------------------------------------------------*/

typedef struct message_t {
  QueElem   elem;            
  void      *contents;
  int       mess_size;
  int       consumed;
} Message_t;

typedef struct channel_t {
  QueHead   *message_list; 
} Channel_t;


/*--------------------------------------------------------------------
| Global Variables
---------------------------------------------------------------------*/

int                channel_capacity;
int                number_of_tasks;
Channel_t          **channel_array;
pthread_mutex_t    single_mutex;
pthread_cond_t     wait_for_free_space;
pthread_cond_t     wait_for_messages;


/*--------------------------------------------------------------------
| Function: createChannel
| Description: Cria e devolve um ponteiro para um canal de comunicação. 
|              Em caso de erro devolve NULL.
---------------------------------------------------------------------*/

Channel_t* createChannel () {
  Channel_t *channel = (Channel_t*) malloc (sizeof(Channel_t));

  if (channel == NULL) {
    fprintf(stderr, "\nErro ao alocar memória para canal\n");
    return NULL;
  }

  channel->message_list = leQueNewHead();

  if (channel->message_list == NULL) {
    fprintf(stderr, "\nErro ao criar lista de mensagens\n");
    return NULL;
  }

  leQueHeadInit (channel->message_list, 0);
  
  return channel;
}


/*--------------------------------------------------------------------
| Function: inicializarMPlib
| Description: Inicializa a MPlib com ntasks*ntasks canais, cada um com
|              capacidade igual a capacidade_de_cada_canal.
|              Em caso de sucesso devolve 0.
|              Em caso de erro devolve -1.
---------------------------------------------------------------------*/


int inicializarMPlib(int capacidade_de_cada_canal, int ntasks) {
  int i, j;
  Channel_t* channel;

  number_of_tasks  = ntasks;
  channel_capacity = capacidade_de_cada_canal;
  channel_array    = (Channel_t**) malloc (sizeof(Channel_t*)*ntasks*ntasks);
  
  if (channel_array == NULL) {
    fprintf(stderr, "\nErro ao alocar memória para MPlib\n");
    return -1;
  }

  if(pthread_mutex_init(&single_mutex, NULL) != 0) {
    fprintf(stderr, "\nErro ao inicializar mutex\n");
    return -1;
  }

  if(pthread_cond_init(&wait_for_free_space, NULL) != 0) {
    fprintf(stderr, "\nErro ao inicializar variável de condição\n");
    return -1;
  }

  if(pthread_cond_init(&wait_for_messages, NULL) != 0){
    fprintf(stderr, "\nErro ao inicializar variável de condição\n");
    return -1;
  }

  for (i=0; i<ntasks; i++) {
    for (j=0; j<ntasks; j++) {
      channel = createChannel();
      if (channel == NULL)
        return -1;

      channel_array[i*number_of_tasks+j] = channel;
    }
  }
  return 0;
}


/*--------------------------------------------------------------------
| Function: libertarMPlib
| Description: Liberta a memória alocada pela MPlib. Devolve void.
|              Em caso de erro, o processo é terminado.
---------------------------------------------------------------------*/

void libertarMPlib() {
  int i,j;
  
  if(pthread_mutex_destroy(&single_mutex) != 0) {
    fprintf(stderr, "\nErro ao destruir mutex\n");
    exit(EXIT_FAILURE);
  }
  
  if(pthread_cond_destroy(&wait_for_free_space) != 0) {
    fprintf(stderr, "\nErro ao destruir variável de condição\n");
    exit(EXIT_FAILURE);
  }

  if(pthread_cond_destroy(&wait_for_messages) != 0) {
    fprintf(stderr, "\nErro ao destruir variável de condição\n");
    exit(EXIT_FAILURE);
  }

  for (i=0; i<number_of_tasks; i++) {
    for (j=0; j<number_of_tasks; j++) {
      Channel_t   *channel = channel_array[i*number_of_tasks+j];
      Message_t   *mess    = (Message_t*) leQueRemFirst(channel->message_list);

      while (mess) {
        if (channel_capacity>0)
          free (mess->contents);
        free (mess);
        mess = (Message_t*) leQueRemFirst(channel->message_list);
      }
      
      /* delete message list header for this channel */
      leQueFreeHead (channel->message_list);
      free (channel);
    }
  }
  free (channel_array);
}


/*--------------------------------------------------------------------
| Function: receberMensagem
| Description: Recebe uma mensagem da tarefaOrig para a tarefaDest,
|              com um dado tamanho máximo, e coloca a mensagem no buffer
|              recebido. Se o canal não estiver vazio, uma mensagem é removida
|              e colocada no buffer. Caso contrário, o recetor é bloqueado
|              até a tarefa de origem enviar mensagens para o canal.
|              Em caso de sucesso devolve o tamanho da mensagem recebida.
|              Em caso de erro devolve -1.
---------------------------------------------------------------------*/

int receberMensagem(int tarefaOrig, int tarefaDest, void *buffer, int tamanho) {
  Channel_t      *channel;
  Message_t      *mess;
  int            copysize;

  if(pthread_mutex_lock(&single_mutex) != 0) {
    fprintf(stderr, "\nErro ao bloquear mutex\n");
    return -1;
  }

  channel = (Channel_t*) channel_array[tarefaDest*number_of_tasks+tarefaOrig];
  mess    = (Message_t*) leQueRemFirst (channel->message_list);

  while (!mess) {
    if(pthread_cond_wait(&wait_for_messages, &single_mutex) != 0) {
        fprintf(stderr, "\nErro ao esperar pela variável de condição\n");
        return -1;
      }
    mess = (Message_t*) leQueRemFirst (channel->message_list);
  }
    
  copysize = (mess->mess_size<tamanho) ? mess->mess_size : tamanho;
  memcpy(buffer, mess->contents, copysize);
  if (channel_capacity >0) {
    free(mess->contents);
    free(mess);
  }
  else
    mess->consumed = 1;
  
  if(pthread_cond_broadcast(&wait_for_free_space) != 0) {
    fprintf(stderr, "\nErro ao desbloquear variável de condição\n");
    return -1;
  }

  if(pthread_mutex_unlock(&single_mutex) != 0) {
    fprintf(stderr, "\nErro ao desbloquear mutex\n");
    return -1;
  }

  return copysize;
}


/*--------------------------------------------------------------------
| Function: enviarMensagem
| Description: Envia uma mensagem, indicada pelo ponteiro msg, da tarefaOrig 
|              para a tarefaDest, com um dado tamanho. Se o canal não estiver
|              cheio, os dados são copiados para um tampão temporário (ou o 
|              emissor fica bloqueado, caso a capacidade do canal seja 0). 
|              Se o canal estiver cheio, o emissor é bloqueado até a tarefa 
|              destino ler mensagens do canal.
|              Em caso de sucesso devolve o tamanho da mensagem.
|              Em caso de erro devolve -1.
---------------------------------------------------------------------*/

int enviarMensagem(int tarefaOrig, int tarefaDest, void *msg, int tamanho) {
  Channel_t      *channel;
  Message_t      *mess;

  mess = (Message_t*) malloc (sizeof(Message_t));

  if(mess == NULL) {
    fprintf(stderr, "\nErro ao alocar memória para mensagem\n");
    return -1;
  }

  leQueElemInit (mess);
  mess->mess_size = tamanho;
  /* if channels are buffered, copy message to a temporary buffer */
  if (channel_capacity >0) {
    mess->contents = malloc (tamanho);

    if(mess->contents == NULL) {
      fprintf(stderr, "\nErro ao alocar memória para mensagem\n");
      return -1;
    }

    memcpy(mess->contents, msg, tamanho);
  }
  /* if channels are not buffered, message will be copied directly from sender to receiver */
  else {
    mess->contents = msg;
    mess->consumed = 0;
  }
  
  if(pthread_mutex_lock(&single_mutex) != 0) {
    fprintf(stderr, "\nErro ao bloquear mutex\n");
    return -1;
  }

  channel = (Channel_t*) channel_array[tarefaDest*number_of_tasks+tarefaOrig];

  /* if channels are buffered, wait until there is buffer available */
  if (channel_capacity >0) {
    while (leQueSize(channel->message_list) >= channel_capacity) {
      if(pthread_cond_wait(&wait_for_free_space, &single_mutex) != 0) {
        fprintf(stderr, "\nErro ao esperar pela variável de condição\n");
        return -1;
      }
    }
  }
  
  leQueInsLast (channel->message_list, mess);
  pthread_cond_broadcast(&wait_for_messages);

  /* if channels are not buffered, wait for message to be read */
  if (channel_capacity==0) {
    while (mess->consumed==0) {
      if(pthread_cond_wait(&wait_for_free_space, &single_mutex) != 0) {
        fprintf(stderr, "\nErro ao esperar pela variável de condição\n");
        return -1;
      }
    
    }
    free(mess);
  }
  
  if(pthread_mutex_unlock(&single_mutex) != 0) {
    fprintf(stderr, "\nErro ao desbloquear mutex\n");
    return -1;
  }

  return tamanho;
}
