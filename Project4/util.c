#include <stdio.h>
#include <stdlib.h>

/*--------------------------------------------------------------------
| Function: die
| Description: Imprime mensagem de erro e termina execucao
---------------------------------------------------------------------*/
void die(char const *reason) {
  fprintf(stderr, "Erro: %s\n", reason);
  exit(-1);
}

/*--------------------------------------------------------------------
| Function: parse_integer_or_exit
| Description: Processa a string str, do parametro name, como um int
---------------------------------------------------------------------*/
int parse_integer_or_exit(char const *str, char const *name, int min_value) {
  int value;
  if (sscanf(str, "%d", &value) != 1) {
    fprintf(stderr, "\nErro no argumento \"%s\".\n\n", name);
    exit(-1);
  }
  if(value < min_value) {
    fprintf(stderr, "\nArgumento \"%s\" tem que ser pelo menos %d", name, min_value);
    exit(-1);
  }
  return value;
}

/*--------------------------------------------------------------------
| Function: parse_double_or_exit
| Description: Processa a string str, do parametro name, como um double
---------------------------------------------------------------------*/
double parse_double_or_exit(char const *str, char const *name, int min_value) {
  double value;
  if (sscanf(str, "%lf", &value) != 1) {
    fprintf(stderr, "\nErro no argumento \"%s\".\n\n", name);
    exit(-1);
  }
  if(value < min_value) {
    fprintf(stderr, "\nArgumento \"%s\" tem que ser pelo menos %d", name, min_value);
    exit(-1);
  }
  return value;
}
