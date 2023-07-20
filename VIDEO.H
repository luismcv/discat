/****************************************************************************/
/*                                 VIDEO.H                                  */
/****************************************************************************/

void modovideo(char modo);
void cls(unsigned char color);
void ventana(int xorigen,int yorigen,int xfin,int yfin,unsigned char color);
void cventana(int yorigen,int yfin,int ancho,unsigned char color);
void print(char *cadena,int x,int y,unsigned char colorpp,
				unsigned char colorf);
void mousecursoron(void);
void mousecursoroff(void);
void rectan(int xorigen,int yorigen,int xfin,int yfin,unsigned char color);
void paleta(unsigned char color,unsigned char rojo,unsigned char verde,
				unsigned char azul);
void fadeout(void);
void fin(void);
