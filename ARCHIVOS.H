/****************************************************************************/
/*                                ARCHIVOS.H                                */
/****************************************************************************/

#include "discat.def"

#define ARCHIVOS

/* Declaramos la estructura de la base de datos */
struct archivos {
	char nombre[9];
	char extension[4];
	char path[21];
	unsigned long longitud;
	union {
		unsigned word;
		struct {
			unsigned int dia : 5;
			unsigned int mes : 4;
			unsigned int year : 7;
		} fecha;
	} fecha;
	union {
		unsigned word;
		struct {
			unsigned int segundos : 5;
			unsigned int minutos : 6;
			unsigned int horas : 5;
		} hora;
	} hora;
	char atributos;
};

struct discos {
	unsigned long byteslibres;
	char nota[31];
	struct archivos archivo[MAXFICHPORDISCO+1];
};

/* Prototypes de las funciones en ARCHIVOS.C */
void load(char *nombrearchivo);
void save(char *nombrearchivo);
void ordenar(void);