/****************************************************************************/
/*                                 SCANER.C                                 */
/*    En este m¢dulo se encuentran las funciones que se encargan de buscar  */
/*  archivos en un disco y rellenar con esta informaci¢n la estructura      */
/*  'disco' y leer y grabar el n£mero de un disco en su n£mero de serie.    */
/*                                                                          */
/****************************************************************************/

#include <dos.h>
#include <dir.h>
#include <stddef.h>
#include <string.h>
#include "discat.def"
#include "scaner.h"
#include "archivos.h"
#include "video.h"

/* De esta forma podemos acceder al puntero *disco
	declarado en la funci¢n main() */
extern struct discos *disco;
int contador;  /* Declaramos un ¡ndice para los archivos en 'disco' */

/* De esta forma podemos acceder a estas dos funciones de DISCAT.C */
	unsigned char cogetecla(void);
	unsigned char testmouse(int *coordx,int *coordy);
	char *unsgtostrd(unsigned unsg,char *str);

void busca_archivos(unsigned char unidad){
	unsigned char unidad_anterior;

	/* Almacenamos la unidad anterior para restaurarla despu‚s */
	unidad_anterior=getdisk();

	contador=0;
	setdisk(unidad);  /* Establece la unidad con la que vamos a trabajar */
	chdir("\\"); /* Cambia al directorio ra¡z */

	busca(unidad);

	/* Identificamos el final de los datos */
	disco->archivo[contador].nombre[0]=0;

	/* Si se sobrepasa el l¡mites de archivos por disco se lo comunicamos
		al usuario */
	if (contador>=MAXFICHPORDISCO) {
		char temp[35],temp2[5]; /* Declaramos algunas variables temporales */

		cventana(80,130,300,145);
		print("­ ATENCION !",14,11,15,145);
		print("S¢lo se pudieron catalogar los",5,13,15,145);
		unsgtostrd(MAXFICHPORDISCO,temp2);
		strcpy(temp,temp2);
		strcat(temp," primeros archivos del disco");
		print(temp,(40-strlen(temp))/2,14,15,145);

      /* Esperamos a que se pulse una tecla o el rat¢n */
		while (cogetecla()==1 && !testmouse(NULL,NULL));
	}

	setdisk(unidad_anterior);  /* Restauramos la unidad anterior */

}

/* Esta funci¢n autorecursiva es la encargada de rastrear todos los
	subdirectorios del disco buscando archivos */
void busca(unsigned char unidad){
	struct ffblk ffblk; /* Define la estructura de control de archivos */
	int hecho; /* Declara una variable que usaremos para averiguar cuando
					no hay m s archivos en el directorio actual */

	hecho = findfirst("*.*",&ffblk,0x37); /* Buscaremos todos las entradas
									del directorio menos las etiquetas de volumen,
									por tanto usamos 37h = 110111b */

	/* El bloque siguiente se ejecutar  mientras haya m s archivos en el disco
	y no se alcanze el m ximo de fichero por disco (MAXFICHPORDISCO) */

	while (!hecho && contador<MAXFICHPORDISCO) {

		/* Si la entrada no es un subdirectorio... */
		if (!(ffblk.ff_attrib & DIRECTORIO)) {
			char ext[5]={ 0,0,0,0,0 };
			char path[MAXPATH];

			/* Separamos el nombre de la extensi¢n */
			fnsplit(ffblk.ff_name,NULL,NULL,disco->archivo[contador].nombre,ext);

			/* Copiamos la extensi¢n sin el punto ( ext[0] ) */
			strcpy(disco->archivo[contador].extension,&ext[1]);

			getcurdir(unidad+1,path); /* Obtenemos el path */
			/* Si el path tiene m s de 20 caracteres no quedamos con los £ltimos
				17 y anteponemos puntos suspensivos '...' */
			if (strlen(path)>20) {
				strcpy(disco->archivo[contador].path,"...");
				strcpy(&disco->archivo[contador].path[3],&path[strlen(path)-17]);
			}
			/* Si no, copiamos el path ¡ntegramente */
			else
				strcpy(disco->archivo[contador].path,path);

			/* Copiamos el resto de los datos del archivo */
			disco->archivo[contador].longitud=ffblk.ff_fsize;
			disco->archivo[contador].fecha.word=ffblk.ff_fdate;
			disco->archivo[contador].hora.word=ffblk.ff_ftime;
			disco->archivo[contador].atributos=ffblk.ff_attrib;

			contador++;  /* Incrementamos el contador */
		}

		/* Si se trata de un subdirectorio... */
		else
			/* Si no es '.' ni '..' ... */
			if ((strcmp(ffblk.ff_name,".")) && (strcmp(ffblk.ff_name,".."))) {
				chdir(ffblk.ff_name);  /* Cambiamos a ‚l */
				busca(unidad);  /* Nos autollamamos */
				chdir(".."); /* Regresamos al directorio padre */
			}
			hecho = findnext(&ffblk); /* Buscamos la siguiente entrada */
	}
}

/* Esta funci¢n lee el n£mero de serie del disco en la unidad 'unidad' */
int lee_numser(unsigned char unidad){
	/* Declaramos una estructura para almacenar el
		'extended BIOS Parameter Block'(BPB) */
	struct {
		unsigned info;
		union {
			unsigned long dword;
			struct {
				unsigned l;
				unsigned h;
			} word;
		} numserie;
		char nombre[11];
		char tipofat[8];
	} data;

	union REGS reg;
	struct SREGS sreg;
	int test;

	/* Funci¢n 69h: Leer/Grabar numero de serie y etiqueta */
	reg.h.ah=0x69;
	reg.h.al=0x00; /* Subfunci¢n 0: Leer */

	/* Sumamos 1 a unidad porque esta funci¢n sigue otro criterio */
	reg.h.bl=++unidad;

	sreg.ds=(unsigned)((unsigned long)&data >>16); /* Una forma de obtener
									el segmento de data sin disponer de FP_SEG */
	reg.x.dx=(unsigned)&data; /* Y as¡ podemos obtener el desplazamiento */

	int86x(0x21,&reg,&reg,&sreg); /* Llamamos a INT 21h */
	if (reg.x.cflag) return(-1); /* Si se ha producido un error retornamos -1*/

	if (unidad==1) test=0xaaaa;
	if (unidad==2) test=0xbbbb;

	/* Si la palabra m s significativa coincide con test, es decir, refleja
	la unidad y la menos significativa est  comprendida entre 0 y MAXDISC
	retornamos ‚sta, que es el n£mero del disco */

	if (data.numserie.word.h==test && data.numserie.word.l >=0
		&& data.numserie.word.l < MAXDISC) return(data.numserie.word.l);
	/* Si no, retornamos -1 */
	else return(-1);
}

/* Esta funci¢n graba el n£mero de serie del disco en la unidad 'unidad' */
int graba_numser(unsigned char unidad,unsigned numdisco){
	/* Declaramos una estructura para almacenar el
		'extended BIOS Parameter Block'(BPB) */
	struct {
		unsigned info;
		union {
			unsigned long dword;
			struct {
				unsigned l;
				unsigned h;
			} word;
		} numserie;
		char nombre[11];
		char tipofat[8];
	} data;
	union REGS reg;
	struct SREGS sreg;

	/* Primero leemos los datos anteriores */

	/* Funci¢n 69h: Leer/Grabar numero de serie y etiqueta */
	reg.h.ah=0x69;
	reg.h.al=0x00; /* Subfunci¢n 0: Leer */

	/* Incrementamos unidad porque esta funci¢n sigue otro criterio */
	reg.h.bl=++unidad;
	sreg.ds=(unsigned)((unsigned long)&data >>16); /* Obtenemos el segmento */
	reg.x.dx=(unsigned)&data; /* Obtenemos el desplazamiento */

	int86x(0x21,&reg,&reg,&sreg); /* Llamamos a INT 21h */
	if (reg.x.cflag) return(-1); /* Si se produce un error retornamos -1 */

	/* Ahora grabamos los datos modificados */

	reg.h.ah=0x69;  /* Funci¢n 69h: Leer/Grabar numero de serie y etiqueta */
	reg.h.al=0x01; /* Subfunci¢n 1: Grabar */
	if (unidad==1) data.numserie.word.h=0xaaaa;
	if (unidad==2) data.numserie.word.h=0xbbbb;
	data.numserie.word.l=numdisco;
	reg.h.bl=unidad;

	int86x(0x21,&reg,&reg,&sreg);

	if (reg.x.cflag) return(-1); /* Si se produce un error retornamos -1 */
	else return(0); /* Si no, retornamos 0 */
}
