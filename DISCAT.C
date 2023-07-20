/****************************************************************************/
/*                                DISCAT.C                                  */
/*    DISCAT es un programa que permite catalogar disquetes para facilitar  */
/* la b£squeda de los ficheros que contienen. Para mayor informaci¢n refe-  */
/* al archivo acompa¤ante DISCAT.TXT.                                       */
/*                                                                          */
/*    DISCAT.C y todos los m¢dulos que lo acompa¤an deben ser compilados en */
/* el modelo de memoria 'Huge'. El c¢digo solo ha sido probado con Borland  */
/* Turbo C++ versi¢n 1.00 en modo ANSI est ndar.                            */
/*                                                                          */
/****************************************************************************/

#include <dir.h>
#include <dos.h>
#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "discat.def"
#include "error.h"
#include "video.h"
#include "archivos.h"
#include "scaner.h"
#include "encuentr.h"

void inicializar(void);
void cabezera(void);
unsigned char cogetecla(void);
unsigned char testmouse(int *coordx,int *coordy);
int getnum(int x,int y,char colorpp,char colorf);
char *getstring(int x,int y,char colorpp,char colorf,int longitud);
char *mayus(char *cadena);
char hazdigito(char p);
char *unsgtostrh(unsigned unsg,char *str);
char *unsgtostrd(unsigned unsg,char *str);
char *ulongtostrd(unsigned long ulong,char *str);
void beep(void);
char pideunidad(void);
void grabadatos(void);
char menu(void);
void catalogar(void);
void buscarnombre(void);
void buscarext(void);
void muestraencontrado(void);
void consultar(void);
void borrardisco(void);

extern struct discos *disco;
unsigned char mousepresent;
char modo_anterior;
char unidad_anterior,direc_anterior[MAXDIR];
char disc[2][MAXDISC]; /* Definimos una matriz para llevar el control
									de los discos que est n catalogados */

extern struct encuentra encontrado[];


void main(void){
	modo_anterior=*(char *)(0x400049L); /* Obtenemos el modo de video actual */
	unidad_anterior=getdisk(); /* la unidad */
	getcwd(direc_anterior,66); /* y el directorio */
	inicializar();

	/* Establecemos nuestro controlador de errores de hardware */
	harderr(errorhandler);

	while (!menu()); /* Se repetir  hasta que se elija 'salir' en menu() */

	fin();
	modovideo(modo_anterior); /* Restablecemos el modo de video anterior */
	setdisk(unidad_anterior); /* la unidad */
	chdir(direc_anterior); /* y el directorio */
	exit(0); /* Salimos y devolvemos 0 al DOS */

}

/* Esta funci¢n realiza algunas inicializaciones y comprobaciones */
void inicializar(void){
	union REGS reg;
	unsigned conta;
	FILE *fichero;

	/* Primero comprobemos que hay una MCGA o VGA color instalada */
	reg.x.ax=0x1a00;
	int86(0x10,&reg,&reg);
	if (!(reg.h.bl==0x8 || reg.h.bl==0xa || reg.h.bl==0xc)){
		printf("Lo siento, pero necesita una MCGA\VGA color.\n");
		exit(1);
	}

	/* Reservamos memoria para estructura 'disco'. Si no hay memoria
	suficiente escribimos un mensaje y salimos */
	if ((disco = (struct discos *)malloc(sizeof(struct discos)))==NULL) {
		printf("No hay memoria sufiente para ejecutar el programa.\n");
		exit(1);
	}

	/* Ahora cargaremos DISCAT.DAT. Si no se pudo abrir... */
	if ((fichero=fopen("discat.dat","rb"))==NULL) {
		char *dirdat,tecla=0;
		char unidad[MAXDRIVE];
		char directorio[MAXDIR];
		int test;
		/* Buscamos CATDIR en las variables de entorno */
		dirdat=mayus(getenv("CATDIR"));
		/* Si no se encontr¢... */
		if (dirdat==NULL) {
			printf("Lo siento, pero no se encuentra DISCAT.DAT"
			 " ni variable de entorno CATDIR.\n");
			exit(1);
		}
		/* Desglosamos el path en sus elementos */
		test=fnsplit(dirdat,unidad,directorio,NULL,NULL);
		/* Si CATDIR contiene una unidad cambiamos a ella */
		if (test & DRIVE) setdisk(unidad[0]-'A');
		/* Si contiene un directorio cambiamos a ‚l */
		if (test & DIRECTORY) chdir(dirdat);
		/* Ahora probamos abrir DISCAT.DAT en el nuevo directorio.
		Si no se encuentra damos opci¢n a crearlo */
		if ((fichero=fopen("discat.dat","rb"))==NULL) {
			printf("No se encuentra fichero DISCAT.DAT. \n");
			printf("¨Desea crearlo? (S/N)");
			while (tecla!='S' && tecla!='s' && tecla!='N' && tecla!='n')
				tecla=getch();
			if (tecla=='S' || tecla=='s'){
				if ((fichero=fopen("discat.dat","wb"))!=NULL) {
					for(conta=0;conta<MAXDISC;conta++) {
						putc(FALSE,fichero);
						putc(FALSE,fichero);
					}
					fclose(fichero);
					if ((fichero=fopen("discat.dat","rb"))==NULL) {
						printf("ERROR. Aunque se ha creado DISCAT.DAT "
						 "no se puede leer.");
						exit(1);
					}
				}
				else {
					printf("Lo siento no se puede crear DISCAT.DAT");
					exit(1);
				}
			}
			else {
				setdisk(unidad_anterior);
				chdir(direc_anterior);
				exit(1);
			}
		}
	}
	/* Leemos los datos contenidos en DISCAT.DAT */
	for(conta=0;conta<MAXDISC;conta++) {
		disc[A][conta]=getc(fichero);
		disc[B][conta]=getc(fichero);
	}
	fclose(fichero);

	/* Debemos inicializar el modo de video antes que el rat¢n */
	modovideo(0x13);
	/* Comprobamos la presencia del rat¢n llamando a la interrupci¢n 33h,
		funci¢n 0 */
	reg.x.ax=0;
	int86(0x33,&reg,&reg);
	/* Si el controlador del rat¢n est  instalado ax retorna FFFFh */
	if (reg.x.ax==0xffff) {
		mousepresent=TRUE; /* Hay un rat¢n instalado */
		reg.x.ax=4;
		reg.x.cx=600; /* Apartamos el cursor a un lado */
		reg.x.dx=180;
		int86(0x33,&reg,&reg);
		mousecursoron();  /* Y lo activamos */
	}
	else
		mousepresent=FALSE;   /* No hay rat¢n instalado */

}

/* CABEZERA. Escribe en pantalla una cabezera */
void cabezera(void){
	cventana(9,28,290,1);
	print("DISCAT",3,2,14,1);
	print("Por Luis Mar¡a Cruz. 1995",12,2,10,1);
}

/* COGETECLA. Lee una tecla si se ha pulsado */
unsigned char cogetecla(void){
	if (kbhit()) return (getch());
	/* Si no se ha pulsado retornamos 1, ya que 0 indica una tecla especial */
	else return(1);
}

/* TESTMOUSE. Retorna 0 si no se ha pulsado ning£n bot¢n y 1 si se ha pulsado
	alguno. Adem s devuelve las coordenadas del cursor cuando se puls¢ */
unsigned char testmouse(int *coordx,int *coordy){
	union REGS reg;

	/* Si no hay un rat¢n instalado regresamos */
	if (!mousepresent) return(0);

	reg.x.ax=0x5; /* Funci¢n 5 */
	reg.x.bx=0x0; /* Comprobamos el bot¢n izquierdo */
	int86(0x33,&reg,&reg); /* Llamamos a INT 33h */
	if (reg.x.bx !=0) {
		*coordx=reg.x.cx/2;   /* El controlador supone una resoluci¢n 640x200
					 as¡ que dividimos x/2 quedando as¡ 320x200 */
		*coordy=reg.x.dx;
		return (1);
	}

	reg.x.ax=0x5;
	reg.x.bx=0x1;  /* Comprobamos el bot¢n derecho */
	int86(0x33,&reg,&reg);
	if (reg.x.bx !=0) {
		*coordx=reg.x.cx/2; /* El controlador supone una resoluci¢n 640x200
					 as¡ que dividimos x/2 quedando as¡ 320x200 */
		*coordy=reg.x.dx;
		return(1);
	}
	return(0);
}

/* GETNUM. Funci¢n que lee del teclado un n£mero de disco de 4 cifras
	en hexadecimal */
int getnum(int x,int y,char colorpp,char colorf){
	char indice=0,tempstring[5];

	/* Dibujamos un recuadro */
	rectan(8*x-3,8*y-3,8*(x+4)+1,8*(y+1)+1,colorf);
	while(1) {
		unsigned char tecla;
		tecla=getch();
		if (tecla==13)
			/* Si se puls¢ INTRO sin ning£n n£mero retornamos -1 */
			if (indice==0 ) return(-1);
			/* Si no, marcamos el final del n£mero y salimos del bucle */
			else {
				tempstring[indice]=0;
				break;
			}
		/* Si se puls¢ ESCAPE retornamos -2 */
		if (tecla==27) return(-2);
		/* Tomamos el digito si est  entre 0 y F */
		if ((tecla>='0' && tecla<='9') || (tecla>='a' && tecla<='f')
		 || (tecla>='A' && tecla<='F'))
			/* Si no hay 4 cifras todav¡a... */
			if (indice<4) {
				char temp[2];
				temp[0]=tecla;
				temp[1]=0;
				print(mayus(temp),x+indice,y,colorpp,colorf);
				tempstring[indice++]=tecla;
			}
			else beep(); /* Si no emitimos un pitido */
		/* Si la tecla es RETROCESO borramos el caracter anterior */
		else if (tecla==8 && indice>0) print(" ",x+(--indice),y,colorpp,colorf);
			  else beep();
	}
	  /* Retornamos el valor del n£mero */
	  return((int)strtol(tempstring, NULL, 16));

}

/* GETSTRING. Lee del teclado una cadena de una determinada longitud */
char *getstring(int x,int y,char colorpp,char colorf,int longitud){
	char indice,tempstring[31];
	indice=0;
	/* Dibujamos un recuadro */
	rectan(8*x-3,8*y-3,8*(x+longitud)+1,8*(y+1)+1,colorf);
	while (1) {
		unsigned char tecla;
		tecla=getch();
		/* Si pulsa INTRO, marcamos el final de la cadena y salimos del bucle */
		if (tecla==13) {
				tempstring[indice]=0;
				break;
		}
		/* Si se puls¢ RETROCESO borramos el car cter anterior */
		if (tecla==8 && indice>0) print(" ",x+(--indice),y,colorpp,colorf);
		/* Si no a¤adimos el car cter a la cadena */
		else
			if (tecla>=' ' && indice<longitud) {
				char temp[2];
				temp[0]=tecla;
				temp[1]=0;
				print(temp,x+indice,y,colorpp,colorf);
				tempstring[indice++]=tecla;
			}
			else beep();
	}
	return(tempstring); /* Retornamos la cadena */
}

/* MAYUS. Convierte todos los caracteres de una cadena a may£sculas */
char *mayus(char *cadena){
	int longitud, conta;

	longitud = strlen(cadena); /* Calculamos la longitud de la cadena */
	for (conta=0; conta<longitud; conta++)
		cadena[conta] = toupper(cadena[conta]);
	return(cadena); /* Retornamos la cadena */
}

/* UNSGTOSTRH. Esta funci¢n convierte un entero sin signo a una cadena,
	en formato hexadecimal. */
char *unsgtostrh(unsigned unsg,char *str){

	str[0]=hazdigito(unsg/4096);
	unsg-=4096*(unsg/4096);
	str[1]=hazdigito(unsg/256);
	unsg-=256*(unsg/256);
	str[2]=hazdigito(unsg/16);
	unsg-=16*(unsg/16);
	str[3]=hazdigito(unsg);
	str[4]=0;

	return(str);
}

/* UNSGTOSTRD. Esta funci¢n convierte un entero sin signo a una cadena,
	en formato decimal. */
char *unsgtostrd(unsigned unsg,char *str){
	char cont=0,n,b,c=FALSE;
	int div=1000;

	for (b=0;b<2;b++) {
		n=unsg/div;
		str[cont]=hazdigito(n);
		unsg-=div*n;
		div/=10;
		/* Si la cifra no es 0... */
		if (n!=0) c=TRUE;
		/* Si la cifra no es 0 o ya hay alguna cifra anterior... */
		if (n!=0 || c) cont++;
	}

	n=unsg/10;
	str[cont]=hazdigito(n);
	unsg-=10*n;
	cont++;

	str[cont]=hazdigito(unsg);
	cont++;
	str[cont]=0;

	return(str);
}

/* ULONGTOSTRD. Esta funci¢n convierte un entero 'long' sin signo a una
	cadena, en formato decimal. */
char *ulongtostrd(unsigned long ulong,char *str){
	char cont=0,n,b,c=FALSE;
	unsigned long div=1000000000L;

	for (b=0;b<8;b++) {
		n=ulong/div;
		str[cont]=hazdigito(n);
		ulong-=div*n;
		div=div/10;
		if (n!=0) c=TRUE;
		if (n!=0 || c) cont++;
	}

	n=ulong/10;
	str[cont]=hazdigito(n);
	ulong-=10*n;
	cont++;

	str[cont]=hazdigito(ulong);
	cont++;
	str[cont]=0;

	return(str);
}

/* Esta funci¢n tranforma un valor de 0 a 15 a los d¡gitos 0-F */
char hazdigito(char p){
	char d;
	if (p>=0 && p<=9)
		d='0'+(p);
	if (p>=10 && p<=15)
		d='A'+(p-10);
	return d;
}


/* BEEP. Usa la funci¢n 2h de la interrupci¢n 21h para mostrar el caracter 7h
	(bell) y producir un pitido */
void beep(void){
	union REGS reg;
	reg.h.ah=0x2;
	reg.h.dl=0x7;
	intdos(&reg,&reg); /* Llamada a INT 21h */
}

/* Da a elegir al usuario entre la unidad A o B */
char pideunidad(void){
	int coordx,coordy;
	char unidad,tecla=0;

	mousecursoroff();
	cventana(80,120,200,45);
	print("Elija Unidad",14,11,56,45);
	rectan(140,102,153,113,98);
	print("A",18,13,37,98);
	rectan(173,102,186,113,98);
	print("B",22,13,37,98);
	mousecursoron();

	unidad=255;
	/* Mientras no se elija A o B... */
	while (unidad!=A && unidad!=B) {
		tecla=cogetecla();
		if (tecla=='A' || tecla=='a') unidad=A;
		if (tecla=='B' || tecla=='b') unidad=B;
		if (tecla==27) return(-1);
		if (testmouse(&coordx,&coordy)) {
			if (coordy>101 && coordy<114){
				if (coordx>139 && coordx<154) unidad=A;
				if (coordx>172 && coordx<187) unidad=B;
			}
		}
	}
	return(unidad);
}

/* GRABADATOS. Graba la matriz disc[][] */
void grabadatos(void){
	FILE *fichero;

	/* Si no se pudo abrir DISCAT.DAT para grabar */
	if ((fichero=fopen("discat.dat","wb"))==NULL) {
		cventana(80,120,300,145);
		print("ERROR,",17,11,15,145);
		print("No se pudo grabar DISCAT.DAT",6,13,15,145);
		while (cogetecla()==1 && !testmouse(NULL,NULL));
		modovideo(modo_anterior);
		setdisk(unidad_anterior);
		chdir(direc_anterior);
		exit(1);
	}
	else {
		unsigned conta;
		/* Grabamos la matriz */
		for(conta=0;conta<MAXDISC;conta++) {
			putc(disc[A][conta],fichero);
			putc(disc[B][conta],fichero);
		}
	}

	fclose(fichero);
}


char menu(void){
	char opcion;
	unsigned char tecla=0;
	int coordx,coordy;

	mousecursoroff(); /* Desactivamos el cursor del rat¢n */

	cls(15); /* Borramos la pantalla */

	cabezera(); /* Escribimos la cabezera */

	/* Y dibujamos el men£ */

	cventana(40,180,280,4);
	rectan(140,53,177,65,60);
	paleta(70,255,255,60); /* Resaltamos la primera opci¢n */
	/* y "apagamos" las dem s */
	for (opcion=1;opcion<6;opcion++) paleta(70+opcion,36,32,100);

	print("MENU",18,7,0,60);
	print("Catalogar disco",13,10,70,4);
	print("Buscar nombre",13,12,71,4);
	print("Buscar extensi¢n",13,14,72,4);
	print("Consultar disco",13,16,73,4);
	print("Eliminar disco",13,18,74,4);
	print("Salir",13,20,75,4);

	/* Si hay un rat¢n activa el cursor */
	mousecursoron();

	opcion=0;  /* Opci¢n por defecto */
	/* Mientras no se pulse INTRO... */
	while (tecla!=13) {
		tecla=cogetecla(); /* Leemos el teclado */
		/* Si se trata de una tecla especial... */
		if (tecla==0) {
			tecla=cogetecla(); /* Tomamos el segundo valor */
			/* Si se trata de cursor arriba o izquierda... */
			if (tecla==72 || tecla==75){
				paleta(70+opcion,36,32,100); /* Apagamos la opci¢n anterior */
				opcion--; /* Decrementamos la opci¢n */
				if (opcion<0) opcion=5; /* Si se pasa el l¡mite saltamos al otro */
				paleta(70+opcion,255,255,60); /* Resaltamos la opci¢n actual */
			}
			/* Si se trata de cursor abajo o derecha... */
			if (tecla==80 || tecla==77) {
				paleta(70+opcion,36,32,100);
				opcion++;
				if (opcion>5) opcion=0;
				paleta(70+opcion,255,255,60);
			}
		}
		/* Si se ha pulsado alg£n bot¢n del rat¢n */
		if (testmouse(&coordx,&coordy))
			/* Comprobamos si se ha pulsado sobre alguna opci¢n, en ese caso
				salimos del bucle con la opci¢n elegida */
			if (coordx>99 && coordx <224) {
				if (coordy>77 && coordy<91) { opcion=0; break; }
				if (coordy>93 && coordy<107) { opcion=1; break; }
				if (coordy>109 && coordy<123) { opcion=2; break; }
				if (coordy>125 && coordy<139) { opcion=3; break; }
				if (coordy>141 && coordy<155) { opcion=4; break; }
				if (coordy>157 && coordy<171) { opcion=5; break; }
			}
	}

	/* Llamamos a la funci¢n correspondiente a cada opci¢n */
	if (opcion==0) catalogar();
	if (opcion==1) buscarnombre();
	if (opcion==2) buscarext();
	if (opcion==3) consultar();
	if (opcion==4) borrardisco();
	if (opcion==5) return(1); /* Si se elige salir retornamos 1 */
	return(0);
}

void catalogar(void){
	int coordx,coordy, numdisco;
	char unidad,nombrearchivo[13],temp[5];
	union REGS reg;

	mousecursoroff();
	cls(7);
	cabezera();
	mousecursoron();

	if ((unidad=pideunidad())==-1) return;

	mousecursoroff();

	/* Llamamos a busca_archivos() para que explore el disco en la unidad */
	busca_archivos(unidad);
	ordenar(); /* Ordenamos los archivos por orden alfab‚tico */

	/* Leemos el numero de serie. Si no es v lido... */

	if ((numdisco=lee_numser(unidad))==-1 || disc[unidad][numdisco]==0) {
		unsigned conta=0;

		/* Buscamos el n£mero del primer disco no catalogado */
		while (disc[unidad][conta]!=0) {
			conta++;
			/* Si se alcanza el m ximo de discos por unidad lo indicamos al
				usuario y regresamos al men£ */
			if (conta>=MAXDISC) {
				cventana(80,120,300,145);
				print("Lo siento,",15,11,15,145);
				print("no hay n£meros libres",10,13,15,145);
				while (cogetecla()==1 && !testmouse(&coordx,&coordy));
				mousecursoron();
				return;
			}
		}

		cventana(70,135,240,67);

		/* Pedimos un n£mero de disco v lido al usuario */
		print("N£mero disco\(0000-",9,10,104,67);
		unsgtostrh(MAXDISC-1,temp);
		print(mayus(temp),27,10,104,67);
		print("\)",31,10,104,67);
		print("Por defecto:",12,12,104,67);
		unsgtostrh(conta,temp);
		mayus(temp);
		print(temp,25,12,104,67);
		/* Pedimos n£meros mientras no se introduzca uno v lido */
		do {
			numdisco=getnum(18,14,104,67);
			/* Si simplemente pulsa INTRO se toma el n£mero por defecto */
			if (numdisco==-1){
				numdisco=conta;
				break;
			}
			/* Si puls¢ ESCAPE regresamos al men£ */
			if (numdisco==-2){
				mousecursoron();
				return;
			}
		} while (numdisco<0 || numdisco>=MAXDISC);

		/* Si el n£mero introducido por el usuario corresponde con un disco
			ya catalogado le pedimos confirmaci¢n */
		if (disc[unidad][numdisco]==TRUE){
			char seguro=FALSE,tecla;
			cventana(78,128,220,45);
			print("Ese n£mero ya est  usado",8,11,56,45);
			print("¨Seguro que quiere usarlo?",7,12,56,45);
			rectan(140,110,153,121,98);
			print("S",18,14,37,98);
			rectan(173,110,186,121,98);
			print("N",22,14,37,98);
			mousecursoron();
			while (seguro!=TRUE) {
				tecla=cogetecla();
				if (tecla=='S' || tecla=='s') seguro=TRUE;
				if (tecla=='N' || tecla=='n' || tecla==27) return;
				if (testmouse(&coordx,&coordy)) {
					if (coordy>110 && coordy<121){
						if (coordx>139 && coordx<154) seguro=TRUE;
						if (coordx>172 && coordx<187) return;
					}
				}
			}
			mousecursoroff();
		}
	}

	/* Leemos los bytes libres del disco */
	reg.h.ah=0x36;
	reg.h.dl=unidad+1;
	intdos(&reg,&reg);
	/* Si ax retorna FFFFh se ha producido un error */
	if (reg.x.ax==0xFFFF) {
		cventana(80,120,300,145);
		print("ERROR,",17,11,15,145);
		print("No se pudo leer bytes libres",6,13,15,145);
		while (cogetecla()==1 && !testmouse(&coordx,&coordy));
		return;
	}
	else disco->byteslibres=(long)reg.x.ax * (long)reg.x.bx * (long)reg.x.cx;

	/* Pedimos una nota para el disco */
	cventana(70,135,260,88);
	print("NOTA para el disco",8,10,104,88);
	unsgtostrh(numdisco,temp);
	mayus(temp);
	if (unidad==A) print("A",27,10,104,88);
	else print("B",27,10,104,88);
	print(temp,32-strlen(temp),10,104,88);
	strcpy(disco->nota,getstring(5,14,104,88,30));

	/* Grabamos el n£mero de serie con su c¢digo en el disco */
	if (lee_numser(unidad)!=numdisco) if (graba_numser(unidad,numdisco)==-1) {
		cventana(65,145,300,145);
		print("No se pudo grabar",12,9,15,145);
		print("el n£mero de serie",11,11,15,145);
		print("Recuerde usar el mismo n£mero",6,14,15,145);
		print("la pr¢xima vez que lo catalogue",5,16,15,145);
		while (cogetecla()==1 && !testmouse(&coordx,&coordy));
	}

	/* Componemos el nombre del archivo de datos */
	strcpy(nombrearchivo,"DISC");
	unsgtostrh(numdisco,temp);
	strcat(nombrearchivo,temp);
	if (unidad==0) strcat(nombrearchivo,".AAA");
	if (unidad==1) strcat(nombrearchivo,".BBB");

	/* Y grabamos los datos en ‚l */
	save(nombrearchivo);

	/* Establecemos el disco como catalogado */
	disc[unidad][numdisco]=TRUE;
	grabadatos();

	mousecursoron();
}


void buscarnombre(void){
	char nombre[9],opcion,tecla=0;
	char unidad;
	int coordx,coordy,cont=0;

	mousecursoroff();
	cls(7);
	cabezera();

	/* Pedimos un archivo a buscar */
	cventana(70,135,250,104);
	print("Introduzca archivo a buscar",6,10,67,104);
	do
		strcpy(nombre,getstring(16,14,67,104,8));
	while (nombre[0]==0); /* As¡ evitamos una cadena en blanco */

	mayus(nombre);

	/* Elegimos el tipo de b£squeda */
	cventana(70,155,250,104);
	print("Tipo de b£squeda",12,10,67,104);
	for (opcion=1;opcion<3;opcion++) paleta(70+opcion,36,32,100);
	opcion=0;  /* Opci¢n por defecto */
	paleta(70+opcion,255,255,60);
	print("Exacta",16,12,70,104);
	print("Normal",16,14,71,104);
	print("Extendida",16,16,72,104);
	mousecursoron();
	while (tecla!=13) {
		tecla=cogetecla();
		if (tecla==0) {
			tecla=cogetecla();
			if (tecla==72 || tecla==75){
				paleta(70+opcion,36,32,100);
				opcion--;
				if (opcion<0) opcion=2;
				paleta(70+opcion,255,255,60);
			}
			if (tecla==80 || tecla==77){
				paleta(70+opcion,36,32,100);
				opcion++;
				if (opcion>2) opcion=0;
				paleta(70+opcion,255,255,60);
			}
		}
		if (tecla==27) return;
		if (testmouse(&coordx,&coordy)) {
			if (coordx>123 && coordx<199){
				if (coordy>93 && coordy<105) { opcion=0;   break; }
				if (coordy>109 && coordy<121) { opcion=1;   break; }
				if (coordy>125 && coordy<137) { opcion=2;   break; }
			}
		}
	}
	/* Ahora buscamos los archivos que correspondan con el patr¢n de b£squeda */
	for (unidad=0;unidad<2;unidad++) {
		int numdisco;
		for (numdisco=0;numdisco<MAXDISC;numdisco++){
			/* Si el disco est  catalogado... */
			if (disc[unidad][numdisco]==TRUE) {
				char nombrearchivo[13],temp[5];
				int conta=0;

				/* Formamos el nombre del archivo que contiene los datos
					del disco */
				strcpy(nombrearchivo,"DISC");
				unsgtostrh(numdisco,temp);
				strcat(nombrearchivo,temp);
				if (unidad==0) strcat(nombrearchivo,".AAA");
				if (unidad==1) strcat(nombrearchivo,".BBB");
				/* Lo leemos. */
				load(nombrearchivo);

				/* Mientras no se llege al final de los ficheros del disco y quede
					espacio en encontrado[]... */
				while (disco->archivo[conta].nombre[0]!=0 && cont<MAXENCUENTRA) {
					char *f;
					/* Si modo EXACTO y los nombres coinciden... */
					if (opcion==0 && !strcmp(disco->archivo[conta].nombre,nombre)){
						/* Rellenamos encontrado[cont] */
						encontrado[cont].archivo=disco->archivo[conta];
						encontrado[cont].unidad=unidad;
						encontrado[cont].numdisco=numdisco;
						cont++;
					}
					/* Buscamos nombre en disco->fichero[conta].nombre */
					f=strstr(disco->archivo[conta].nombre,nombre);

					/* Si modo NORMAL, y disco->fichero[conta].nombre comienza
						por nombre, es decir, si ambos punteros se¤alan a la misma
						direcci¢n de memoria... */
					if (opcion==1 && (disco->archivo[conta].nombre==f)) {
						/* Rellenamos encontrado[cont] */
						encontrado[cont].archivo=disco->archivo[conta];
						encontrado[cont].unidad=unidad;
						encontrado[cont].numdisco=numdisco;
						cont++;
					}
					/* Si modo EXTENDIDO y se encontr¢ nombre en
						disco->fichero[conta].nombre... */
					if (opcion==2 && f) {
						/* Rellenamos encontrado[cont] */
						encontrado[cont].archivo=disco->archivo[conta];
						encontrado[cont].unidad=unidad;
						encontrado[cont].numdisco=numdisco;
						cont++;
					}
				conta++; /* Pasamos al siguiente fichero del disco */
				}
			}
		}
	}

	mousecursoroff();

	encontrado[cont].archivo.nombre[0]=0; /* Marcamos el final de encontrado */

	/* Si se agot¢ el espacio en encontrado[] indicamos al usuario que s¢lo se
		mostrar n los MAXENCUENTRA primeros ficheros encontrados */
	if (cont>=MAXENCUENTRA) {
		char temp[35],temp2[5];
		cventana(80,138,300,145);
		print("­ ATENCION !",14,11,15,145);
		print("Demasiadas correspondencias.",6,13,15,145);
		strcpy(temp,"S¢lo se mostrar n las ");
		unsgtostrd(MAXENCUENTRA,temp2);
		strcat(temp,temp2);
		strcat(temp," primeras.");
		print(temp,(40-strlen(temp))/2,15,15,145);
		while (cogetecla()==1 && !testmouse(&coordx,&coordy));
	}
	muestraencontrado(); /* Mostramos los ficheros que hemos encontrado */
}

void buscarext(void){
	char extension[4];
	char unidad;
	int cont=0;

	mousecursoroff();
	cls(7);
	cabezera();

	/* Solicitamos una extensi¢n */
	cventana(70,135,270,104);
	print("Introduzca extensi¢n a buscar",4,10,67,104);
	do
		strcpy(extension,getstring(18,14,67,104,3));
	while (extension[0]==0);

	mayus(extension);

	/* Registramos todos los registros buscando la extensi¢n */
	for (unidad=0;unidad<2;unidad++) {
		int numdisco;
		/* Ahora buscamos las correspondencias */
		for (numdisco=0;numdisco<MAXDISC;numdisco++){
			if (disc[unidad][numdisco]==TRUE) {
				char nombrearchivo[13],temp[5];
				int conta=0;

				/* Formamos el nombre del archivo que contiene los datos
					del disco */
				strcpy(nombrearchivo,"DISC");
				unsgtostrh(numdisco,temp);
				strcat(nombrearchivo,temp);
				if (unidad==0) strcat(nombrearchivo,".AAA");
				if (unidad==1) strcat(nombrearchivo,".BBB");

				load(nombrearchivo);

				while (disco->archivo[conta].nombre[0]!=0 && cont<MAXENCUENTRA) {
					/* Si coinciden las extensiones */
					if (!strcmp(disco->archivo[conta].extension,extension)){
						/* Rellenamos encontrado[cont] */
						encontrado[cont].archivo=disco->archivo[conta];
						encontrado[cont].unidad=unidad;
						encontrado[cont].numdisco=numdisco;
						cont++;
					}
					conta++;
				}
			}
		}
	}

	encontrado[cont].archivo.nombre[0]=0; /* Marcamos el final */

	/* Si se agot¢ el espacio en encontrado[] indicamos al usuario que s¢lo se
		mostrar n los MAXENCUENTRA primeros ficheros encontrados */
	if (cont>=MAXENCUENTRA) {
		char temp[35],temp2[5];
		cventana(80,138,300,145);
		print("­ ATENCION !",14,11,15,145);
		print("Demasiadas correspondencias.",6,13,15,145);
		strcpy(temp,"S¢lo se mostrar n las ");
		unsgtostrd(MAXENCUENTRA,temp2);
		strcat(temp,temp2);
		strcat(temp," primeras.");
		print(temp,(40-strlen(temp))/2,15,15,145);
		while (cogetecla()==1 && !testmouse(NULL,NULL));
	}
	muestraencontrado(); /* Mostramos los ficheros encontrados */

}

void muestraencontrado(void){
	int coordx,coordy,num=0,n=0;
	char inc;

	cls(7);
	cabezera();

	/* Calculamos el n£mero de ficheros en encontrado[] */
	while (encontrado[n].archivo.nombre[0]!=0) n++;

	/* Si n=0, es que no se encontr¢ ninguno que correspondiese al
		patr¢n de b£squeda */
	if (n==0) {
		cventana(80,120,300,145);
		print("No se encontr¢.",12,12,15,145);
		while (cogetecla()==1 && !testmouse(&coordx,&coordy));
		mousecursoron();
		return;
	}
	n--; /* Decrementamos n para obtener el ¡ndice del £ltimo fichero
			  en encontrado */

	/* Dibujamos la ficha */
	cventana(38,48,120,1);
	cventana(48,150,280,1);
	print("PATH",4,7,45,1);
	rectan(82,53,291,65,9);
	print("FECHA",4,9,45,1);
	rectan(82,69,170,81,9);
	print("HORA",23,9,45,1);
	rectan(222,69,291,81,9);
	print("LONGITUD",4,11,45,1);
	rectan(101,85,184,97,9);
	print("DISCO",25,11,45,1);
	rectan(245,85,291,97,9);
	print("ATRIBUTOS",16,13,45,1);

	/* y las opciones */
	cventana(156,185,240,89);
	rectan(45,165,113,177,95);
	print("ANTERIOR",6,21,145,95);
	rectan(133,165,178,177,95);
	print("SALIR",17,21,145,95);
	rectan(197,165,272,177,95);
	print("SIGUIENTE",25,21,145,95);

	while (1){
		char temp[15];

		/* Unimos el nombre y la extensi¢n */
		strcpy(temp,encontrado[num].archivo.nombre);
		strcat(temp,".");
		strcat(temp,encontrado[num].archivo.extension);
		/* y los mostramos */
		print("            ",14,5,45,1);
		print(temp,(40-strlen(temp))/2,5,45,1);

		/* mostramos el resto de los datos */
		unsgtostrh(encontrado[num].numdisco,temp);
		mayus(temp);
		if (encontrado[num].unidad==A) print("A",31,11,10,9);
		else print("B",31,11,10,9);
		print(temp,32,11,10,9);
		print("\\                     ",11,7,10,9);
		print(encontrado[num].archivo.path,12,7,10,9);
		print("  /  /    ",11,9,10,9);
		unsgtostrd(encontrado[num].archivo.fecha.fecha.dia,temp);
		print(temp,13-strlen(temp),9,10,9);
		unsgtostrd(encontrado[num].archivo.fecha.fecha.mes,temp);
		print(temp,16-strlen(temp),9,10,9);
		unsgtostrd(encontrado[num].archivo.fecha.fecha.year+1980,temp);
		print(temp,21-strlen(temp),9,10,9);
		print("  :  :  ",28,9,10,9);
		unsgtostrd(encontrado[num].archivo.hora.hora.horas,temp);
		print(temp,30-strlen(temp),9,10,9);
		unsgtostrd(encontrado[num].archivo.hora.hora.minutos,temp);
		print(temp,33-strlen(temp),9,10,9);
		unsgtostrd(encontrado[num].archivo.hora.hora.segundos*2,temp);
		print(temp,36-strlen(temp),9,10,9);
		print("          ",13,11,10,9);
		ulongtostrd(encontrado[num].archivo.longitud,temp);
		print(temp,23-strlen(temp),11,10,9);

		/* Si el archivo tiene un determinado atributo ‚ste aparecer  resaltado,
			si no aparecer  "apagado" */
		if (encontrado[num].archivo.atributos & SOLOLECTURA)
			print("Solo lectura",6,15,10,1);
		else
			print("Solo lectura",6,15,108,1);

		if (encontrado[num].archivo.atributos & OCULTO)
			print("Oculto",6,17,10,1);
		else
			print("Oculto",6,17,108,1);

		if (encontrado[num].archivo.atributos & SISTEMA)
			print("Sistema",27,15,10,1);
		else
			print("Sistema",27,15,108,1);

		if (encontrado[num].archivo.atributos & ARCHIVO)
			print("Archivo",27,17,10,1);
		else
			print("Archivo",27,17,108,1);

		inc=0;
		mousecursoron();
		/* Mientras no se elija ANTERIOR ni SIGUIENTE... */
		while (inc==0) {
			unsigned tecla=cogetecla(); /* Leemos el teclado */
			/* Si se trata de una tecla especial... */
			if (tecla==0) {
				tecla=cogetecla(); /* Leemos el segundo valor */
				/* Si es cursor arriba o izquierda, inc=-1 */
				if (tecla==72 || tecla==75) inc=-1;
				/* Si es cursor abajo o derecha, inc=+1 */
				if (tecla==80 || tecla==77) inc=1;
			}
			/* Si se ha pulsado ESCAPE regresamos */
			if (tecla==27) return;
			/* Si se ha pulsado el rat¢n */
			if (testmouse(&coordx,&coordy)) {
				/* Comprobamos que se haya hecho sobre alguna opci¢n, y actuamos
					en consecuencia */
				if (coordy>165 && coordy<177){
					if (coordx>45 && coordx<113) inc=-1;
					if (coordx>133 && coordx<178) return;
					if (coordx>197 && coordx<272) inc=+1;
				}
			}
		}
		num+=inc; /* Incrementamos o decrementamos num seg£n la opci¢n elegida */
		/* Si ya no hay m s ficheros que mostrar en un determinado sentido damos
			un pitido */
		if (num>n) {
			num=n;
			beep();
		}
		if (num<0) {
			num=0;
			beep();
		}
	}

}

void consultar(void){
	int coordx,coordy;
	char unidad;
	char tecla=0,inc,temp[25],nombrearchivo[13];
	int numdisco,num=0,n=0;

	mousecursoroff();
	cls(7);
	cabezera();
	mousecursoron();

	/* Pedimos la unidad */
	if ((unidad=pideunidad())==-1) return;

	/* y el n£mero de disco */
	mousecursoroff();
	cventana(70,135,240,67);
	print("N£mero disco\(0000-",9,10,104,67);
	unsgtostrh(MAXDISC-1,temp);
	print(mayus(temp),31-strlen(temp),10,104,67);
	print("\)",31,10,104,67);
	do {
		numdisco=getnum(18,14,104,67);
		if (numdisco==-2){
			mousecursoron();
			return;
		}
	} while (numdisco<0 || numdisco>=MAXDISC);

	/* Si el disco no est  catalogado lo notificamos al usuario */
	if (disc[unidad][numdisco]!=TRUE) {
		cventana(80,120,300,145);
		print("Lo siento,",15,11,15,145);
		print("ese disco no est  catalogado",6,13,15,145);
		while (cogetecla()==1 && !testmouse(&coordx,&coordy));
		mousecursoron();
		return;
	}

	/* Formamos el nombre del archivo que contiene los datos del disco */
	strcpy(nombrearchivo,"DISC");
	unsgtostrh(numdisco,temp);
	strcat(nombrearchivo,temp);
	if (unidad==0) strcat(nombrearchivo,".AAA");
	if (unidad==1) strcat(nombrearchivo,".BBB");

	load(nombrearchivo);

	/* Escribimos algunos datos del disco */
	cventana(65,140,270,1);
	print("Disco",14,9,45,1);
	unsgtostrd(numdisco,temp);
	mayus(temp);
	if (unidad==A) print("A000",21,9,45,1);
	else print("B000",21,9,45,1);
	print(temp,26-strlen(temp),9,45,1);
	print(disco->nota,(40-strlen(disco->nota))/2,12,45,1);
	ulongtostrd(disco->byteslibres,temp);
	strcat(temp," bytes libres.");
	print(temp,(40-strlen(temp))/2,15,45,1);
	while (cogetecla()==1 && !testmouse(&coordx,&coordy));

	/* Calculamos el n£mero de ficheros en el disco */
	while (disco->archivo[n].nombre[0]!=0) n++;
	/* Si n=0 es que el disco est  vac¡o */
	if (n==0) {
		cls(7);
		cabezera();
		cventana(80,120,300,145);
		print("El disco est  vac¡o.",10,12,15,145);
		while (cogetecla()==1 && !testmouse(&coordx,&coordy));
		mousecursoron();
		return;
	}
	n--; /* Decrementamos n para obtener el ¡ndice del ultimo
			  fichero catalogado */

	/* Dibujamos la ficha */
	cventana(38,48,120,1);
	cventana(48,150,280,1);
	print("PATH",4,7,45,1);
	rectan(82,53,291,65,9);
	print("FECHA",4,9,45,1);
	rectan(82,69,170,81,9);
	print("HORA",23,9,45,1);
	rectan(222,69,291,81,9);
	print("LONGITUD",4,11,45,1);
	rectan(101,85,184,97,9);
	print("ATRIBUTOS",16,13,45,1);

	/* y las opciones */
	cventana(156,185,240,89);
	rectan(45,165,113,177,95);
	print("ANTERIOR",6,21,145,95);
	rectan(133,165,178,177,95);
	print("SALIR",17,21,145,95);
	rectan(197,165,272,177,95);
	print("SIGUIENTE",25,21,145,95);

	while (1){
		/* Unimos el nombre y la extensi¢n */
		strcpy(temp,disco->archivo[num].nombre);
		strcat(temp,".");
		strcat(temp,disco->archivo[num].extension);
		/* y lo imprimimos */
		print("            ",14,5,45,1);
		print(temp,(40-strlen(temp))/2,5,45,1);

		/* mostramos el resto de los datos */
		rectan(82,53,291,65,9);
		print("\\                     ",11,7,10,9);
		print(disco->archivo[num].path,12,7,10,9);
		print("  /  /    ",11,9,10,9);
		unsgtostrd(disco->archivo[num].fecha.fecha.dia,temp);
		print(temp,13-strlen(temp),9,10,9);
		unsgtostrd(disco->archivo[num].fecha.fecha.mes,temp);
		print(temp,16-strlen(temp),9,10,9);
		unsgtostrd(disco->archivo[num].fecha.fecha.year+1980,temp);
		print(temp,21-strlen(temp),9,10,9);
		print("  :  :  ",28,9,10,9);
		unsgtostrd(disco->archivo[num].hora.hora.horas,temp);
		print(temp,30-strlen(temp),9,10,9);
		unsgtostrd(disco->archivo[num].hora.hora.minutos,temp);
		print(temp,33-strlen(temp),9,10,9);
		unsgtostrd(disco->archivo[num].hora.hora.segundos*2,temp);
		print(temp,36-strlen(temp),9,10,9);
		print("          ",13,11,10,9);
		ulongtostrd(disco->archivo[num].longitud,temp);
		print(temp,23-strlen(temp),11,10,9);

		if (disco->archivo[num].atributos & SOLOLECTURA)
			print("Solo lectura",6,15,10,1);
		else
			print("Solo lectura",6,15,108,1);

		if (disco->archivo[num].atributos & OCULTO)
			print("Oculto",6,17,10,1);
		else
			print("Oculto",6,17,108,1);

		if (disco->archivo[num].atributos & SISTEMA)
			print("Sistema",27,15,10,1);
		else
			print("Sistema",27,15,108,1);

		if (disco->archivo[num].atributos & ARCHIVO)
			print("Archivo",27,17,10,1);
		else
			print("Archivo",27,17,108,1);

		inc=0;
		mousecursoron();
		while (inc==0) {
			tecla=cogetecla();
			if (tecla==0) {
				tecla=cogetecla();
				if (tecla==72 || tecla==75) inc=-1;
				if (tecla==80 || tecla==77) inc=1;
			}
			if (tecla==27) return;
			if (testmouse(&coordx,&coordy)) {
				if (coordy>165 && coordy<177){
					if (coordx>45 && coordx<113) inc=-1;
					if (coordx>133 && coordx<178) return;
					if (coordx>197 && coordx<272) inc=+1;
				}
			}
		}
		num+=inc;
		if (num>n) {
			num=n;
			beep();
		}
		if (num<0) {
			num=0;
			beep();
		}
	}
}

void borrardisco(void){
	int coordx,coordy;
	char unidad;
	char tecla=0,seguro=FALSE,nombrearchivo[13],temp[5];
	int numdisco;

	mousecursoroff();
	cls(7);
	cabezera();
	mousecursoron();

	/* Pedimos la unidad a la que pertence el disco a borrar */
	if ((unidad=pideunidad())==-1) return;

	/* y su n£mero */
	mousecursoroff();
	cventana(70,135,240,67);
	print("N£mero disco\(0000-",9,10,104,67);
	unsgtostrh(MAXDISC-1,temp);
	print(mayus(temp),31-strlen(temp),10,104,67);
	print("\)",31,10,104,67);
	do {
		numdisco=getnum(18,14,104,67);
		if (numdisco==-2){
			mousecursoron();
			return;
		}
	} while (numdisco<0 || numdisco>=MAXDISC);

	/* Si el disco no est  catalogado l¢gicamente no se podr  eliminar... */
	if (disc[unidad][numdisco]!=TRUE) {
		cventana(80,120,300,145);
		print("Lo siento,",15,11,15,145);
		print("ese disco no est  catalogado",6,13,15,145);
		while (cogetecla()==1 && !testmouse(&coordx,&coordy));
		mousecursoron();
		return;
	}

	/* Pedimos al usuario que confirme la acci¢n */
	cventana(80,120,200,45);
	print("¨Est  seguro?",14,11,56,45);
	rectan(140,102,153,113,98);
	print("S",18,13,37,98);
	rectan(173,102,186,113,98);
	print("N",22,13,37,98);
	mousecursoron();
	while (seguro!=TRUE) {
		tecla=cogetecla();
		if (tecla=='S' || tecla=='s') seguro=TRUE;
		if (tecla=='N' || tecla=='n' || tecla==27) return;
		if (testmouse(&coordx,&coordy)) {
			if (coordy>101 && coordy<114){
				if (coordx>139 && coordx<154) seguro=TRUE;
				if (coordx>172 && coordx<187) return;
			}
		}
	}

	/* Formamos el nombre del archivo que contiene los datos del disco */
	strcpy(nombrearchivo,"DISC");
	unsgtostrh(numdisco,temp);
	strcat(nombrearchivo,temp);
	if (unidad==0) strcat(nombrearchivo,".AAA");
	if (unidad==1) strcat(nombrearchivo,".BBB");

	/* Establecemos el disco como no catalogado */
	disc[unidad][numdisco]=FALSE;
	grabadatos();

	/* Y finalmente, borramos el archivo en el que est  almacenado su
		contenido. Si no se puede borrar se lo decimos al usuario */

	if (remove(nombrearchivo) != 0){
		mousecursoroff();
		cventana(80,120,300,145);
		print("Lo siento, no se pudo borrar",6,11,15,145);
		print(nombrearchivo,(40-strlen(nombrearchivo))/2,13,15,145);
		while (cogetecla()==1 && !testmouse(&coordx,&coordy));
		mousecursoron();
		return;
	}
}
