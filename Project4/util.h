/*
// Funcoes diversas
// Sistemas Operativos, DEI/IST/ULisboa 2017-18
*/

#ifndef UTIL_H
#define UTIL_H

/*--------------------------------------------------------------------
| Function: die
| Description: Imprime mensagem de erro e termina execucao
---------------------------------------------------------------------*/
void die(char const *reason);

/*--------------------------------------------------------------------
| Function: parse_integer_or_exit
| Description: Processa a string str, do parametro name, como um int
---------------------------------------------------------------------*/
int parse_integer_or_exit(char const *str, char const *name, int min_value);

/*--------------------------------------------------------------------
| Function: parse_double_or_exit
| Description: Processa a string str, do parametro name, como um double
---------------------------------------------------------------------*/
double parse_double_or_exit(char const*str, char const *name, int min_value);

#endif
