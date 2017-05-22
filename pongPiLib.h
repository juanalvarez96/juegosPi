#ifndef _PONGPILIB_H_
#define _PONGPILIB_H_

#include <stdio.h>

// CONSTANTES DEL JUEGO
#define MATRIZ_ANCHO_PONGPI	10
#define MATRIZ_ALTO_PONGPI 	7
#define RAQUETA_ANCHO_PONGPI	1
#define RAQUETA_ALTO_PONGPI 	3

typedef struct {
	// Posicion
	int x;
	int y;
	// Forma
	int ancho;
	int alto;
} tipo_raqueta_pongPi;

typedef struct {
	// Posicion
	int x;
	int y;
	// Trayectoria
	int xv;
	int yv;
} tipo_pelota_pongPi;

typedef struct {
	// Matriz de ocupacion de las distintas posiciones que conforman el display
	// (correspondiente al estado encendido/apagado de cada uno de los leds)
	int matriz[MATRIZ_ANCHO_PONGPI][MATRIZ_ALTO_PONGPI];
} tipo_pantalla_pongPi;

typedef struct {
  tipo_pantalla_pongPi pantalla;
  tipo_raqueta_pongPi raquetaIzq;
  tipo_raqueta_pongPi raquetaDer;
  tipo_pelota_pongPi pelota;
} tipo_pongPi;

extern tipo_pantalla_pongPi pantalla_inicial_pongPi;
//------------------------------------------------------
// FUNCIONES DE INICIALIZACION / RESET
//------------------------------------------------------
void ReseteaMatrizPongPi (tipo_pantalla_pongPi *p_pantalla);
void ReseteaPelotaPongPi (tipo_pelota_pongPi *p_pelota);
void ReseteaRaquetaDer (tipo_raqueta_pongPi *p_raqueta);
void ReseteaRaquetaIzq (tipo_raqueta_pongPi *p_raqueta);

//------------------------------------------------------
// FUNCIONES DE VISUALIZACION (ACTUALIZACION DEL OBJETO PANTALLA QUE LUEGO USARA EL DISPLAY)
//------------------------------------------------------
void PintaMensajeInicialPantallaPongPi (tipo_pantalla_pongPi *p_pantalla, tipo_pantalla_pongPi *p_pantalla_inicial);
void PintaPantallaPorTerminalPongPi (tipo_pantalla_pongPi *p_pantalla);
void PintaRaquetaPongPi (tipo_raqueta_pongPi *p_raqueta, tipo_pantalla_pongPi *p_pantalla);
void PintaPelotaPongPi (tipo_pelota_pongPi *p_pelota, tipo_pantalla_pongPi *p_pantalla);
void ActualizaPantallaPongPi (tipo_pongPi* p_pongPi);
void InicializaPongPi (tipo_pongPi *p_pongPi);

#endif /* _PONGPILIB_H_ */

