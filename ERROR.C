/****************************************************************************/
/*                                 ERROR.C                                  */
/*    En este m¢dulo se encuentra nuestro controlador de errores de         */
/* hardware.                                                                */
/*                                                                          */
/****************************************************************************/

#include <string.h>
#include <dir.h>
#include <dos.h>
#include "discat.def"
#include "error.h"
#include "video.h"

/* Declaramos una estructura con los mensajes de error del DOS */
static char *mensaje_de_error[] = {
	"Protecci¢n contra escritura",
	"Unidad desconocida",
	"La unidad no est  lista",
	"Comando desconocido",
	"Error de datos (CRC error)",
	"Petici¢n inv lidad",
	"Error de b£squeda",
	"Tipo de medio desconocido",
	"Sector no se encontr¢",
	"Impresora sin papel",
	"Fallo de escritura",
	"Fallo de lectura",
	"Fallo general",
	"Reservado",
	"Reservado",
	"Cambio de disco inv lido"
};

	/* Necesitamos acceder a las tres siguientes variables para restaurar
		el modo de video y el directorio de trabajo */
	extern char modo_anterior;
	extern char unidad_anterior,direc_anterior[MAXDIR];

	/* De esta forma podemos acceder a estas dos funciones de DISCAT.C */
	unsigned char cogetecla(void);
	unsigned char testmouse(int *coordx,int *coordy);

/* La siguiente directiva hace que el compilador no presente avisos
	por no usar los par metros ax,bp, y si.  */
#pragma warn -par
int errorhandler(int errval,int ax,int bp,int si) {

	int coordx,coordy;
	unsigned tecla=0,opcion=0;

	/* Mostramos el mensaje de error y esperamos una respuesta del usuario */

	mousecursoroff();
	paleta(70,255,255,60); /* Establecemos el color de la opci¢n resaltada */
	paleta(71,36,32,100); /* y de la otra */

	cventana(70,130,240,6);

	print(mensaje_de_error[errval & 0x00FF],(40-
		strlen(mensaje_de_error[errval & 0x00FF]))/2,10,89,6);

	print("Reintentar",8,13,70,6);
	print("Cancelar",24,13,71,6);

	mousecursoron();

	while (tecla!=13) {
		tecla=cogetecla();
		if (tecla==0) {
			tecla=cogetecla();
			/* Si se pulsan los cursores... */
			if (tecla==72 || tecla==75 || tecla==80 || tecla==77){
				paleta(70+opcion,36,32,100); /* Apagamos la opci¢n anterior */
				opcion=opcion^0x1; /* Cambiamos a la otra opci¢n */
				paleta(70+opcion,255,255,60); /* Resaltamos la nueva opci¢n */
			}
		}

		/* Si se ha pulsado el rat¢n... */
		if (testmouse(&coordx,&coordy)) {
			/* Averiguamos si ha sido sobre alguna opci¢n
				y actuamos en consecuencia*/
			if (coordy>101 && coordy<112){
				if (coordx>61 && coordx<142) { opcion=0;   break; }
				if (coordx>189 && coordx<261) { opcion=1;   break; }
			}
		}
	}

	/* Si se elige reintentar se lo indicamos al DOS */
	if (opcion==0) hardresume(REINTENTAR);

	/* Si se ha elegido cancelar... */
	if (opcion==1) {
		modovideo(modo_anterior); /* Restauramos el modo de video, */
		setdisk(unidad_anterior); /* la unidad, */
		chdir(direc_anterior); /* y el directorio anteriores */
		/* Salimos del programa a traves de la interrupci¢n 23h */
		hardresume(CANCELAR);
	}
	return(CANCELAR);
}
/* Volvemos a activar las advertencias */
#pragma warn +par
