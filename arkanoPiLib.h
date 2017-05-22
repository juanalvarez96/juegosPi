#ifndef _ARKANOPILIB_H_
#define _ARKANOPILIB_H_

#include <stdio.h>

// CONSTANTES DEL JUEGO
#define MATRIZ_ANCHO 	10
#define MATRIZ_ALTO 	7
#define LADRILLOS_ANCHO 10
#define LADRILLOS_ALTO 	2
#define RAQUETA_ANCHO 		3
#define RAQUETA_ALTO 		1

typedef struct {
	// Posicion
	int x;
	int y;
	// Forma
	int ancho;
	int alto;
} tipo_raqueta_arkanoPi;

typedef struct {
	// Posicion
	int x;
	int y;
	// Trayectoria
	int xv;
	int yv;
} tipo_pelota_arkanoPi;

typedef struct {
	// Matriz de ocupación de las distintas posiciones que conforman el display
	// (correspondiente al estado encendido/apagado de cada uno de los leds)
	int matriz[MATRIZ_ANCHO][MATRIZ_ALTO];
} tipo_pantalla_arkanoPi;

typedef struct {
  tipo_pantalla_arkanoPi ladrillos; // Notese que, por simplicidad, los ladrillos comparten tipo con la pantalla
  tipo_pantalla_arkanoPi pantalla;
  tipo_raqueta_arkanoPi raqueta;
  tipo_pelota_arkanoPi pelota;
} tipo_arkanoPi;

extern tipo_pantalla_arkanoPi pantalla_inicial;

//------------------------------------------------------
// FUNCIONES DE INICIALIZACION / RESET
//------------------------------------------------------
void ReseteaMatriz(tipo_pantalla_arkanoPi *p_pantalla);
void ReseteaLadrillos(tipo_pantalla_arkanoPi *p_ladrillos);
void ReseteaPelota(tipo_pelota_arkanoPi *p_pelota);
void ReseteaRaqueta(tipo_raqueta_arkanoPi *p_raqueta);

//------------------------------------------------------
// FUNCIONES DE VISUALIZACION (ACTUALIZACION DEL OBJETO PANTALLA QUE LUEGO USARA EL DISPLAY)
//------------------------------------------------------
void PintaMensajeInicialPantalla(tipo_pantalla_arkanoPi *p_pantalla, tipo_pantalla_arkanoPi *p_pantalla_inicial);
void PintaPantallaPorTerminal(tipo_pantalla_arkanoPi *p_pantalla);
void PintaLadrillos(tipo_pantalla_arkanoPi *p_ladrillos, tipo_pantalla_arkanoPi *p_pantalla);
void PintaRaqueta(tipo_raqueta_arkanoPi *p_raqueta, tipo_pantalla_arkanoPi *p_pantalla);
void PintaPelota(tipo_pelota_arkanoPi *p_pelota, tipo_pantalla_arkanoPi *p_pantalla);
void ActualizaPantalla(tipo_arkanoPi* p_arkanoPi);

void InicializaArkanoPi(tipo_arkanoPi *p_arkanoPi);
int CalculaLadrillosRestantes(tipo_pantalla_arkanoPi *p_ladrillos);

#endif /* _ARKANOPILIB_H_ */
