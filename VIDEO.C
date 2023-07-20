/****************************************************************************/
/*                                 VIDEO.C                                  */
/*    Este m¢dulo se encarga de todas las funciones de video como dibujar   */
/*  ventanas, cambiar el modo de video, escribir texto, cambiar la paleta,  */
/*  realizar fades y activar y desactivar el cursor del rat¢n.              */
/*                                                                          */
/****************************************************************************/

#include <dos.h>
#include <stddef.h>
#include "video.h"
#include "discat.def"

char *screen=(char *)0xa0000000L; /* puntero a la memoria de video */

/* De esta forma podemos acceder a estas dos funciones de DISCAT.C */
	unsigned char cogetecla(void);
	unsigned char testmouse(int *coordx,int *coordy);

/* MODOVIDEO. Establece el modo grafico */
void modovideo(char modo){
	union REGS reg;
	reg.h.ah = 0;  /* funci¢n 0 */
	reg.h.al = modo;
	int86(0x10,&reg,&reg); /* llamamos a INT 10h */
}

/* CLS. Borra la pantalla a un determinado color escribiendo directamente
	la memoria de video */
void cls(unsigned char color){
	unsigned a;
	for (a=0;a<64000L;a++) screen[a]=color;
}

/* VENTANA. Dibuja una ventana.
				 xorigen,yorigen = coord. esquina superior izquierda
				 xfin,yfin = coord. esquina inferior dcha.
				 color = color del fondo */
void ventana(int xorigen,int yorigen,int xfin,int yfin,unsigned char color){
	int x,y;

	/* Rellenamos la ventana */
	for (y=yorigen;y<yfin;y++)
		for (x=xorigen;x<xfin;x++) screen[320*y+x]=color;

	for (x=xorigen;x<xfin;x++) {
		screen[320*yorigen+x]=0; /* Dibujamos el borde superior */
		screen[320*yfin+x]=0; /* y el inferior */
		/* Dibujamos la sombra inferior */
		screen[320*(yfin+1)+x+5]=24;
		screen[320*(yfin+2)+x+5]=24;
		screen[320*(yfin+3)+x+5]=24;
	}

	for (y=yorigen;y<yfin+1;y++) {
		screen[320*y+xorigen]=0; /* Borde izquierdo */
		screen[320*y+xfin]=0; /* Borde derecho */
		/* Y ahora la sombra lateral */
		screen[320*(y+3)+xfin+1]=24;
		screen[320*(y+3)+xfin+2]=24;
		screen[320*(y+3)+xfin+3]=24;
		screen[320*(y+3)+xfin+4]=24;
	}
}

/* CVENTANA.  Calcula los par metros de una ventana centrada
	horizontalmente y llama a VENTANA para que la dibuje */
void cventana(int yorigen,int yfin,int ancho,unsigned char color){
	int xorigen=(319 - ancho)/2;
	int xfin= xorigen + ancho;
	ventana(xorigen,yorigen,xfin,yfin,color);
}

/* PRINT. Escribe una cadena de texto en (x,y) usando el BIOS.
				colorpp = color primer plano
				colorf  = color fondo        */
void print(char *cadena,int x,int y,unsigned char colorpp,
				unsigned char colorf){

	union REGS reg;
	int conta=0;

	/* Mientras no se acabe la cadena... */
	while( cadena[conta]!=0 ){
		/* Colocamos el cursor */
		reg.h.ah=2; /* Funci¢n 2 */
		reg.h.bh=0; /* BH=0 para los modos gr ficos */
		reg.h.dh=y; /* columna */
		reg.h.dl=x+conta; /* fila */
		int86(0x10,&reg,&reg); /* Llamamos a INT 10h */

		/* Imprimimos un car cter */
		reg.h.ah=9; /* Funci¢n 9 */
		reg.h.al=cadena[conta]; /* AH=c¢digo ASCII del car cter */
		reg.h.bh=colorf;
		reg.h.bl=colorpp;
		reg.x.cx=1; /* S¢lo escribimos un car cter */
		int86(0x10,&reg,&reg); /* INT 10h */
		conta++; /* Pasamos al siguiente car cter de la cadena */
	}
}

/* MOUSECURSORON.  Activa el cursor del rat¢n llamando a la
	interrupci¢n 33h funci¢n 1 */
void mousecursoron(void){
	union REGS reg;
	extern char mousepresent; /* Esta variable est  declarada en DISCAT.C */
	/* Si no hay un rat¢n instalado regresamos */
	if (!mousepresent) return;
	reg.x.ax=1;
	int86(0x33,&reg,&reg);
}

/* MOUSECURSOROFF. Desactiva el cursor del rat¢n llamando a la
	interrupci¢n 33h funci¢n 2 */
void mousecursoroff(void){
	union REGS reg;
	extern char mousepresent;
	/* Si no hay un rat¢n instalado regresamos */
	if (!mousepresent) return;
	reg.x.ax=2;
	int86(0x33,&reg,&reg);
}

/* RECTAN. Dibuja un rect ngulo. Es igual que ventana() salvo que no dibuja
	la sombra */
void rectan(int xorigen,int yorigen,int xfin,int yfin,unsigned char color){
	int x,y;
	/* Rellenamos la ventana */
	for (y=yorigen;y<yfin;y++)
		for (x=xorigen;x<xfin;x++) screen[320*y+x]=color;

	/* Dibujamos los bordes horizontales... */
	for (x=xorigen;x<xfin;x++) {
		screen[320*yorigen+x]=0;
		screen[320*yfin+x]=0;
	}
	/* y los verticales */
	for (y=yorigen;y<yfin+1;y++) {
		screen[320*y+xorigen]=0;
		screen[320*y+xfin]=0;
	}
}

/* PALETA. Establece el color 'numcolor' de la paleta */
void paleta(unsigned char color,unsigned char rojo,
				unsigned char verde,unsigned char azul){
	outportb(0x3c8,color); /* Seleccionamos el color a cambiar */
	/* y pasamos los valores rojo,verde y azul (RGB) */
	outportb(0x3c9,rojo);
	outportb(0x3c9,verde);
	outportb(0x3c9,azul);
}


/* Las siguientes funciones se encargan de la pantalla de cierre. Las rutinas
	que realizan los fundidos solo trabajan con los 80 primeros colores de
	la paleta, puesto que son los £nicos que se usan en la pantalla de men£
	y cierre */

void fadeout(void){
	char rojo,verde,azul,conta;
	int color;
	for (conta=0;conta<64;conta++) {

		/* Esperamos al siguiente 'vertical retrace' */
		while (!(inportb(0x3da) & 0x8));

		for (color=0;color<80;color++) {
			outportb(0x3c7,color);
			rojo=inportb(0x3c9);
			verde=inportb(0x3c9);
			azul=inportb(0x3c9);
			if (rojo>0) rojo--;
			if (verde>0) verde--;
			if (azul>0) azul--;
			outportb(0x3c8,color);
			outportb(0x3c9,rojo);
			outportb(0x3c9,verde);
			outportb(0x3c9,azul);
		}
	}
}

void fin(void){
	char rojo,verde,azul,conta;
	int color;
	/* Definimos una matriz donde almacenaremos los primeros 80 colores
		de la paleta actual */
	struct color {
		char rojo;
		char verde;
		char azul;
	};
	struct color paleta[80];

	for (color=0;color<80;color++) {
		outportb(0x3c7,color);
		paleta[color].rojo=inportb(0x3c9);
		paleta[color].verde=inportb(0x3c9);
		paleta[color].azul=inportb(0x3c9);
	}

	/* Hacemos un fundido de la pantalla */
	fadeout();

	/* Escribimos la pantalla de cierre */
	cls(7);
	cventana(20,180,310,1);
	print("DISCAT",17,4,45,1);
	print(" Escrito por Luis Mar¡a Cruz V zquez",2,6,15,1);
	print("en el verano del 95 para el concurso",2,7,15,1);
	print("de programaci¢n en C  organizado por",2,8,15,1);
	print("PC ACTUAL.",2,9,15,1);
	print(" Agradecimientos a  David Jurgen por",2,12,15,1);
	print("su  HelpPC, y a Coronado Enterprises",2,13,15,1);
	print("por su Tutor de C. Ambos me han sido",2,14,15,1);
	print("de gran ayuda.",2,15,15,1);
	print(" Y por  supuesto a  PC ACTUAL,  Ache",2,17,15,1);
	print("Sistemas y Database DM.",2,18,15,1);

	/* Realizamos un fade in de la pantalla */
	for (conta=0;conta<64;conta++) {

		/* Esperamos al siguiente 'vertical retrace' para evitar esos puntitos
			que aparecen al realizar el fade */
		while (!(inportb(0x3da) & 0x8));

		for (color=0;color<80;color++) {
			outportb(0x3c7,color);
			rojo=inportb(0x3c9);
			verde=inportb(0x3c9);
			azul=inportb(0x3c9);
			if (rojo<paleta[color].rojo) rojo++;
			if (verde<paleta[color].verde) verde++;
			if (azul<paleta[color].azul) azul++;
			outportb(0x3c8,color);
			outportb(0x3c9,rojo);
			outportb(0x3c9,verde);
			outportb(0x3c9,azul);
		}
	}

	/* Esperamos a que se pulse una tecla o el rat¢n */
	while (cogetecla()==1 && !testmouse(NULL,NULL));

	/* Hacemos otro fundido */
	fadeout();
}