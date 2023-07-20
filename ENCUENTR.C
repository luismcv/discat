/****************************************************************************/
/*                                ENCUENTR.C                                */
/*    En este m¢dulo s¢lo se declara la estructura que albergar  los datos  */
/* de los archivos encontrados por "Buscar nombre" y "Buscar extensi¢n".    */
/*                                                                          */
/****************************************************************************/

#include "encuentr.h"
#include "discat.def"

/* Tenemos que definir encontrado[] en un m¢dulo a parte para evitar
el error "Too much global data defined in file", es decir, para evitar
que los datos globales excedan de 64Kb en el m¢dulo DISCAT */

struct encuentra encontrado[MAXENCUENTRA+1];