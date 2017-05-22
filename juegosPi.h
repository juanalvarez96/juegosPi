#ifndef _PONGPI_H_
#define _PONGPI_H_

#include <time.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <wiringPi.h>

#include "fsm.h" // maquinas de estados
#include "tmr.h" // timers

#include "arkanoPiLib.h"
#include "pongPiLib.h"

#define CLK_MS 10 // PERIODO ACTUALIZACION MAQUINA ESTADOS

typedef struct {
	tipo_arkanoPi arkanoPi;
	tipo_pongPi pongPi;
	char teclaPulsada;
} tipo_juego;

// FLAGS
#define FLAG_SEL_1 0x01
#define FLAG_SEL_2 0x02
#define FLAG_PELOTA_ARKANOPI 0x04
#define FLAG_PELOTA_PONGPI 0x08
#define FLAG_RAQUETA_JOYSTICK 0x10
#define FLAG_RAQUETA_IZQUIERDA 0x20
#define FLAG_RAQUETA_DERECHA 0x40
#define FLAG_RAQUETA_IZQUIERDA_1 0x80
#define FLAG_RAQUETA_DERECHA_1 0x100
#define FLAG_RAQUETA_IZQUIERDA_2 0x200
#define FLAG_RAQUETA_DERECHA_2 0x400
#define FLAG_TECLADO_MATRICIAL 0x800
#define FLAG_CAMBIO_DIFICULTAD 0x1000
#define FLAG_FINAL_JUEGO 0x2000


// Lock/Unlock
#define FLAGS_KEY 1
#define STD_IO_BUFFER_KEY 2

//------------------------------------------------------
// FUNCIONES DE ENTRADA === TRANSICION
//------------------------------------------------------
int CompruebaTeclaPulsada(fsm_t* this);
int CompruebaRaqueta(fsm_t* this);
int CompruebaRaquetaIzquierda(fsm_t* this);
int CompruebaRaquetaDerecha(fsm_t* this);
int CompruebaRaquetaIzquierda1(fsm_t* this);
int CompruebaRaquetaDerecha1(fsm_t* this);
int CompruebaRaquetaIzquierda2(fsm_t* this);
int CompruebaRaquetaDerecha2(fsm_t* this);
int CompruebaPelotaArkanoPi(fsm_t* this);
int CompruebaPelotaPongPi(fsm_t* this);
int CompruebaFinalJuego(fsm_t* this);
int CompruebaPausa(fsm_t* this);
int CompruebaSelJuego1(fsm_t* this);
int CompruebaSelJuego2(fsm_t* this);
int CompruebaCambioJuego(fsm_t* this);
int CompruebaCambioDificultad(fsm_t* this);
//------------------------------------------------------
// FUNCIONES DE ACCION === SALIDA
//------------------------------------------------------

void InicializaJuego (void);
void MueveRaqueta(void);
void MueveRaquetaIzquierda (void);
void MueveRaquetaDerecha (void);
void MueveRaquetaIzquierda1 (void);
void MueveRaquetaDerecha1 (void);
void MueveRaquetaIzquierda2 (void);
void MueveRaquetaDerecha2 (void);
void MovimientoPelotaArkanoPi (void);
void MovimientoPelotaPongPi (void);
void FinalJuego (void);
void ReseteaJuego (void);
void PausaJuego (void);
void SeleccionJuego1(void);
void SeleccionJuego2(void);
void CambioDificultad(void);
void PreparaCambio(void);

//------------------------------------------------------
// FUNCIONES DE INICIALIZACION
//------------------------------------------------------
int systemSetup (void);
void fsm_setup(fsm_t* fsm);

//------------------------------------------------------
// SUBRUTINAS DE ATENCION A LAS INTERRUPCIONES
//------------------------------------------------------
PI_THREAD(thread_teclado_matricial);
PI_THREAD(thread_leds);
PI_THREAD(thread_explora_teclado);

float lectura_ADC(void);

#endif /* PONGPI_H_ */
