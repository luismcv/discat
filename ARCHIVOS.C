/****************************************************************************/
/*                                ARCHIVOS.C                                */
/*    En este m¢dulo se encuentran las funciones que cargan, graban y orde- */
/* nar los datos de los diquetes catalogados.                               */
/*                                                                          */
/****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <dir.h>
#include <stdlib.h>
#include "archivos.h"
#include "video.h"

/* Definimos un puntero a la estructura de la base de datos */
struct discos *disco;

/* Necesitamos acceder a las tres siguientes variables para restaurar
   el modo de video y el directorio de trabajo */
extern char modo_anterior;
extern char unidad_anterior,direc_anterior[MAXDIR];

/* De esta forma podemos acceder a estas dos funciones de DISCAT.C */
unsigned char cogetecla(void);
unsigned char testmouse(int *coordx,int *coordy);

/* LOAD. Esta funci¢n carga los datos almacenados en el archivo
			 'nombrearchivo'. */
void load(char *nombrearchivo){
	int cont=0; /* Declaramos un contador */
	struct archivos temp; /* Declaramos una estructura para almacenar
									 temporalmente los datos de un fichero */
	FILE *stream;

	/* Si no se puede abrir 'nombrearchivo'... */
	if ((stream = fopen(nombrearchivo, "rb")) == NULL) {
		/* Necesitamos acceder a las tres siguientes variables para restaurar
			el modo de video y el directorio de trabajo */
		extern char modo_anterior;
		extern char unidad_anterior,direc_anterior[];
		char temp[35];

		cventana(80,120,300,145);
		print("ERROR",17,11,15,145);
		strcpy(temp,"No se pudo leer ");
		strcat(temp,nombrearchivo);
		print(temp,(40-strlen(temp))/2,13,15,145);
		while (cogetecla()==1 && !testmouse(NULL,NULL));
		modovideo(modo_anterior);
		setdisk(unidad_anterior);
		chdir(direc_anterior);
		exit(1);
	}

	/* Leemos los byteslibres y la nota del disco */
	fread(&disco->byteslibres,sizeof(disco->byteslibres),1,stream);
	fread(&disco->nota,sizeof(disco->nota),1,stream);

	/* Y ahora leemos los datos correspondientes a los archivos catalogados */
	while (1) {
		/* Cargamos los datos de un archivo en temp */
		fread(&temp, sizeof(temp), 1, stream);

		/* Y lo a¤adimos a la estructura disco */
		disco->archivo[cont]=temp;

		/* Si no hay m s datos identificamos el final
			de la matriz discos->archivo[] */
		if (feof(stream)) {
			disco->archivo[cont].nombre[0]=0;
			break;
		}

		cont++; /* Incrementamos el contador */
	};

	fclose(stream); /* cerramos el fichero */
}

/* SAVE. Esta funci¢n graba los datos en el archivo 'nombrearchivo'. */
void save(char *nombrearchivo){
	int cont=0; /* Declaramos un contador */
	struct archivos temp; /* Declaramos una estructura para almacenar
									teporalmente los datos de un fichero */
	FILE *stream;

	/* Si no se puede abrir el archivo... */
	if ((stream = fopen(nombrearchivo, "wb")) == NULL) {
		/* Necesitamos acceder a las tres siguientes variables para restaurar
			el modo de video y el directorio de trabajo */
		extern char modo_anterior;
		extern char unidad_anterior,direc_anterior[];
		char temp[35];

		cventana(80,120,300,145);
		print("ERROR",17,11,15,145);
		strcpy(temp,"No se pudo grabar ");
		strcat(temp,nombrearchivo);
		print(temp,(40-strlen(temp))/2,13,15,145);
		while (cogetecla()==1 && !testmouse(NULL,NULL));
		modovideo(modo_anterior);
		setdisk(unidad_anterior);
		chdir(direc_anterior);
		exit(1);
	}

	/* Grabamos los bytes libres y la nota del disco */
	fwrite(&disco->byteslibres,sizeof(disco->byteslibres),1,stream);
	fwrite(&disco->nota,sizeof(disco->nota),1,stream);

	/* Mientras queden datos... */
	while (disco->archivo[cont].nombre[0]!=0) {
		/* Copiamos la informaci¢n de un archivo a temp */
		temp=disco->archivo[cont];

      /* grabamos la estructura temp */
		fwrite(&temp, sizeof(temp), 1, stream);

		cont++; /* Incrementamos el contador */
		}

	fclose(stream); /* cerramos el fichero */
}

/* ORDENAR(). Ordena los archivos de 'disco' por orden alfab‚tico mediante
	el m‚todo de la burbuja, es decir, se compara cada nombre con el siguiente,
	si el primero es mayor que el segundo se intercambian. As¡ hasta el £ltimo;
	de esta forma el nombre mayor queda el £ltimo. Ahora se repite la operaci¢n
	hasta el pen£ltimo, despu‚s hasta el antepen£ltimo y as¡ sucesivamente hasta
	que se acaben los nombres o no se realize ning£n cambio. */
void ordenar(void){
	unsigned char cambio=TRUE;
	unsigned n=0;
	unsigned conta;
	struct archivos temp;

   /* Calculamos el n£mero de registros */
	while (disco->archivo[n].nombre[0]!=0) n++;

	/* Mientras haya alg£n cambio y n>0... */
	while (cambio && n>0){
		cambio=FALSE;
		for (conta=1;conta<n;conta++) {
			if (strcmp(disco->archivo[conta-1].nombre,
				disco->archivo[conta].nombre)>0)
				{
				temp=disco->archivo[conta-1];
				disco->archivo[conta-1]=disco->archivo[conta];
				disco->archivo[conta]=temp;
				cambio=TRUE;
			}
			if (strcmp(disco->archivo[conta-1].nombre,disco->archivo[conta].nombre)
				==0 && ((strcmp(disco->archivo[conta-1].extension,
				disco->archivo[conta].extension))>0))
				{
				temp=disco->archivo[conta-1];
				disco->archivo[conta-1]=disco->archivo[conta];
				disco->archivo[conta]=temp;
				cambio=TRUE;
			}
		}
		n--;
	}
}
