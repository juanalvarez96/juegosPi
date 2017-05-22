#include <wiringPi.h>
#include <stdlib.h>
#include "juegosPi.h"

//------------------------------------------------------
// DECLARACION DE PINES Y VARIABLES GLOBALES UTILIZADAS
//------------------------------------------------------
#define FILA1_TECLADO_MATRICIAL 5 // Pin fisico IN: 1
#define COLUMNA1_TECLADO MATRICIAL 0 // Pines fisicos OUT: {1,2}
#define COLUMNA2_TECLADO MATRICIAL 1

//#define PULSADOR_RAQUETA_DER 16 // ARKANOPI solo en caso de problemas con joystick
//#define PULSADOR_RAQUETA_IZQ 19 // Pin fisico IN: 8
#define PULSADOR_RAQUETA_J1_DER 20 // PONGPI
#define PULSADOR_RAQUETA_J1_IZQ 21 // Pines fisicos IN: {14,15,16,17,6,7}
#define PULSADOR_RAQUETA_J2_DER 26
#define PULSADOR_RAQUETA_J2_IZQ 27
#define PULSADOR_QUIT 15
#define PULSADOR_CAMBIO_DIF 16

// Variables utilizadas para el refresco de leds mediante registro de desplazamiento
#define SHCP 4 // Shift register clock input
#define DS 7 // Serial data input

#define SPI_ADC_CH 0
#define SPI_ADC_FREQ 1000000

#define DIFICULTAD_FACIL 500 // Tiempos para los timers utilizados en ms
#define DIFICULTAD_MEDIA 300
#define DIFICULTAD_DIFICIL 100
#define TIEMPO_MUESTRA 30

//#define VERBOSE 1 // Se usa para verificar el buen funcionamiento del ADC
//#define __MODO_DEBUG_TERMINAL__ 1 // Se usa en caso de problemas con los leds
//#define __MODO_DEBUG_TECLADO__ 1 // Se usa en caso de problemas con los pulsadores
//#define __MODO_ARKANOPI_NORMAL__ 1 // Se usa en caso de problemas con el joystick del ArkanoPi
//#define __MODO_LEDS_NORMAL__ 1 // Se usa en caso de problemas con la optimizacion de leds

enum fsm_state { // Declaracion de estados de la maquina utilizada
	WAIT_SEL, WAIT_START, WAIT_PUSH, WAIT_END, WAIT_PAUSE
};

int gpio_input[7] = {5,15,16,20,21,26,27}; // Entradas utilizadas
int gpio_output[13] = {0,1,2,3,4,7,14,17,18,22,23,24,25}; // Salidas utilizadas
int leds_columna[4] = {14,17,18,22}; // Pines fisicos OUT: {15,16,17,18}
int leds_fila[7] = {2,3,4,7,23,24,25}; // Pines fisicos OUT: {3,4,5,6,19,20,21}

volatile int flags = 0;
int dif = 0; // Variable de dificultad del juego
int sel = 0; // Variable de selección del juego
int winner = 1; // Jugador que ha ganado la partida de pongPi
#ifdef __MODO_LEDS_NORMAL__
int contador_col = 0; // Contador usado para el encendido de leds
#endif
#ifndef __MODO_LEDS_NORMAL__
int contador_col = 1; // Contador usado para el encendido de leds con registro desplazamiento
#endif
float voltaje_med; // Medida del voltaje vía ADC
int matrizBienvenida[10][7] = { // Matriz de bienvenida del menu de juego
		{0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0},
		{0,1,1,0,1,0,0},
		{0,1,1,0,0,1,0},
		{0,0,0,0,0,1,0},
		{0,0,0,0,0,1,0},
		{0,1,1,0,0,1,0},
		{0,1,1,0,1,0,0},
		{0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0},
};

tmr_t* tmr_JOYSTICK; // Timer usado para el uso del joystick
tmr_t* tmr_PELOTA; // Timer usado para el movimiento de la pelota

static volatile tipo_juego juego;

// espera hasta activacion reloj
void delay_until(unsigned int next) {
	unsigned int now = millis();
	if(next > now) {
		delay(next - now);
	}
}

//------------------------------------------------------
// FUNCIONES DE LA MAQUINA DE ESTADOS
//------------------------------------------------------

int CompruebaCambioJuego(fsm_t* this) {
	int result;

	piLock(FLAGS_KEY);
	result = (flags & FLAG_SEL_1) || (flags & FLAG_SEL_2);
	piUnlock(FLAGS_KEY);

	return result;
}

int CompruebaSelJuego1(fsm_t* this) {
	int result;

	piLock(FLAGS_KEY);
	result = (flags & FLAG_SEL_1);
	piUnlock(FLAGS_KEY);

	return result;
}

int CompruebaSelJuego2(fsm_t* this) {
	int result;

	piLock(FLAGS_KEY);
	result = (flags & FLAG_SEL_2);
	piUnlock(FLAGS_KEY);

	return result;
}

int CompruebaTeclaPulsada(fsm_t* this) {
	int result;

	piLock(FLAGS_KEY);
#ifndef __MODO_ARKANOPI_NORMAL__
	result =(flags & FLAG_RAQUETA_IZQUIERDA_1) || (flags & FLAG_RAQUETA_DERECHA_1) ||  (flags & FLAG_RAQUETA_IZQUIERDA_2) || (flags & FLAG_RAQUETA_DERECHA_2);
#endif

#ifdef __MODO_ARKANOPI_NORMAL__
	result =(flags & FLAG_RAQUETA_IZQUIERDA_1) || (flags & FLAG_RAQUETA_DERECHA_1) ||  (flags & FLAG_RAQUETA_IZQUIERDA_2) || (flags & FLAG_RAQUETA_DERECHA_2) ||
			(flags & FLAG_RAQUETA_IZQUIERDA) || (flags & FLAG_RAQUETA_DERECHA);
#endif
	piUnlock(FLAGS_KEY);

	return result;
}

#ifndef __MODO_ARKANOPI_NORMAL__
int CompruebaRaqueta(fsm_t* this) {
	int result;
	piLock(FLAGS_KEY);
	result = (flags & FLAG_RAQUETA_JOYSTICK);
	piUnlock(FLAGS_KEY);

	return result;
}
#endif

#ifdef __MODO_ARKANOPI_NORMAL__
int CompruebaRaquetaIzquierda(fsm_t* this) {
	int result;

	piLock(FLAGS_KEY);
	result = (flags & FLAG_RAQUETA_IZQUIERDA);
	piUnlock(FLAGS_KEY);

	return result;
}

int CompruebaRaquetaDerecha(fsm_t* this) {
	int result;

	piLock(FLAGS_KEY);
	result = (flags & FLAG_RAQUETA_DERECHA);
	piUnlock(FLAGS_KEY);

	return result;
}
#endif

int CompruebaRaquetaIzquierda1(fsm_t* this) {
	int result;

	piLock(FLAGS_KEY);
	result = (flags & FLAG_RAQUETA_IZQUIERDA_1);
	piUnlock(FLAGS_KEY);

	return result;
}

int CompruebaRaquetaDerecha1(fsm_t* this) {
	int result;

	piLock(FLAGS_KEY);
	result = (flags & FLAG_RAQUETA_DERECHA_1);
	piUnlock(FLAGS_KEY);

	return result;
}

int CompruebaRaquetaIzquierda2(fsm_t* this) {
	int result;

	piLock(FLAGS_KEY);
	result = (flags & FLAG_RAQUETA_IZQUIERDA_2);
	piUnlock(FLAGS_KEY);

	return result;
}

int CompruebaRaquetaDerecha2(fsm_t* this) {
	int result;

	piLock(FLAGS_KEY);
	result = (flags & FLAG_RAQUETA_DERECHA_2);
	piUnlock(FLAGS_KEY);

	return result;
}

int CompruebaPelotaArkano(fsm_t* this) {
	int result;

	piLock(FLAGS_KEY);
	result = (flags & FLAG_PELOTA_ARKANOPI);
	piUnlock(FLAGS_KEY);

	return result;
}

int CompruebaPelotaPongPi(fsm_t* this) {
	int result;

	piLock(FLAGS_KEY);
	result = (flags & FLAG_PELOTA_PONGPI);
	piUnlock(FLAGS_KEY);

	return result;
}

int CompruebaFinalJuego(fsm_t* this) {
	int result;

	piLock(FLAGS_KEY);
	result = (flags & FLAG_FINAL_JUEGO);
	piUnlock(FLAGS_KEY);

	return result;
}

int CompruebaPausa(fsm_t* this) {
	int result;

	piLock(FLAGS_KEY);
#ifndef __MODO_ARKANOPI_NORMAL__
	result = ((flags & FLAG_RAQUETA_IZQUIERDA_1) && (flags & FLAG_RAQUETA_DERECHA_1)) ||  ((flags & FLAG_RAQUETA_IZQUIERDA_2) && (flags & FLAG_RAQUETA_DERECHA_2));
#endif
#ifdef __MODO_ARKANOPI_NORMAL__
	result = ((flags & FLAG_RAQUETA_IZQUIERDA_1) && (flags & FLAG_RAQUETA_DERECHA_1)) ||  ((flags & FLAG_RAQUETA_IZQUIERDA_2) && (flags & FLAG_RAQUETA_DERECHA_2))
			|| ((flags & FLAG_RAQUETA_IZQUIERDA) && (flags & FLAG_RAQUETA_DERECHA));
#endif
	piUnlock(FLAGS_KEY);

	return result;
}

int CompruebaCambioDificultad(fsm_t* this) {
	int result;

	piLock(FLAGS_KEY);
	result = (flags & FLAG_CAMBIO_DIFICULTAD);
	piUnlock(FLAGS_KEY);

	return result;
}

//--------------------------------------------------------
// SUBRUTINAS DE ATENCION A LAS INTERRUPCIONES TEMPORALES
//--------------------------------------------------------

static void timer_isr_pelota(union sigval arg) {
	if ( sel == 1 ) { // Juego 1: queremos mover pelota del arkanoPi
		piLock(FLAGS_KEY);
		flags |= FLAG_PELOTA_ARKANOPI;
		piUnlock(FLAGS_KEY);
	}
	else if( sel == 2 ) { // Juego 2: queremos mover pelota del pongPi
		piLock(FLAGS_KEY);
		flags |= FLAG_PELOTA_PONGPI;
		piUnlock(FLAGS_KEY);
	}
	if( dif == 0 ){ // Modo dificultad facil
		tmr_startms(tmr_PELOTA, DIFICULTAD_FACIL);
	}
	else if( dif == 1 ){ // Modo dificultad medio
		tmr_startms(tmr_PELOTA, DIFICULTAD_MEDIA);
	}
	else if( dif == 2 ){ // Modo dificultad dificil
		tmr_startms(tmr_PELOTA, DIFICULTAD_DIFICIL);
	}
	else {// Trazas para depurar código en caso de error
		printf("Error en isr timer pelota\n");
	}
}

static void timer_isr_joystick(union sigval arg) {
	voltaje_med = lectura_ADC(); // Tomamos varias medidas de la tensión de entrada
	delayMicroseconds(50); // y hacemos una media
	voltaje_med += lectura_ADC();
	delayMicroseconds(50);
	voltaje_med += lectura_ADC();
	delayMicroseconds(50);
	voltaje_med += lectura_ADC();
	delayMicroseconds(50);
	voltaje_med += lectura_ADC();
	delayMicroseconds(50);
	voltaje_med /= 5; // 5 muestras en total
	piLock(FLAGS_KEY);
	flags |= FLAG_RAQUETA_JOYSTICK; // Queremos mover raqueta ArkanoPi
	piUnlock(FLAGS_KEY);
	tmr_startms(tmr_JOYSTICK,TIEMPO_MUESTRA); // Volvemos a arrancar el timer
}

//------------------------------------------------------
// SUBRUTINAS DE ATENCION A LAS INTERRUPCIONES PULSADAS
//------------------------------------------------------

#ifdef __MODO_ARKANOPI_NORMAL__
static void boton_isr_left(void) {
	piLock(FLAGS_KEY);
	flags |= FLAG_RAQUETA_IZQUIERDA; // Queremos mover raqueta ArkanoPi a la izq.
	piUnlock(FLAGS_KEY);
}

static void boton_isr_right(void) {
	piLock(FLAGS_KEY);
	flags |= FLAG_RAQUETA_DERECHA; // Queremos mover raqueta ArkanoPi a la der.
	piUnlock(FLAGS_KEY);
}
#endif

static void boton_isr_left_1(void) {
	piLock(FLAGS_KEY);
	flags |= FLAG_RAQUETA_IZQUIERDA_1; // Queremos mover raqueta J1 PongPi a la izq.
	piUnlock(FLAGS_KEY);
}

static void boton_isr_right_1(void) {
	piLock(FLAGS_KEY);
	flags |= FLAG_RAQUETA_DERECHA_1; // Queremos mover raqueta J1 PongPi a la der.
	piUnlock(FLAGS_KEY);
}

static void boton_isr_left_2(void) {
	piLock(FLAGS_KEY);
	flags |= FLAG_RAQUETA_IZQUIERDA_2; // Queremos mover raqueta J2 PongPi a la izq.
	piUnlock(FLAGS_KEY);
}

static void boton_isr_right_2(void) {
	piLock(FLAGS_KEY);
	flags |= FLAG_RAQUETA_DERECHA_2; // Queremos mover raqueta J2 PongPi a la der.
	piUnlock(FLAGS_KEY);
}

static void boton_isr_quit(void){
	exit(0); // Forzamos fin ejecucion del programa
}

static void boton_isr_dif(void) {
	piLock(FLAGS_KEY);
	flags |= FLAG_CAMBIO_DIFICULTAD; // Queremos cambiar de dificultad
	piUnlock(FLAGS_KEY);
}

//------------------------------------------------------
// FUNCIONES DE ACCION
//------------------------------------------------------

// void SeleccionJuego1 (void): funcion encargada de seleccionar ArkanoPi como
// modo de juego. Para ello tiene que inicializarlo y verificar que todos los flags
// correspondientes a las teclas estan a '0' para poder ver la pantalla inicial.
void SeleccionJuego1(void) {
	piLock (FLAGS_KEY); // Ponemos flags a '0' para evitar que se salte la pantalla inicial
	flags &= ~FLAG_SEL_1; // del estado WAIT_START
	flags &= ~FLAG_RAQUETA_IZQUIERDA;
	flags &= ~FLAG_RAQUETA_DERECHA;
	flags &= ~FLAG_RAQUETA_IZQUIERDA_1;
	flags &= ~FLAG_RAQUETA_DERECHA_1;
	flags &= ~FLAG_RAQUETA_IZQUIERDA_2;
	flags &= ~FLAG_RAQUETA_DERECHA_2;
	piUnlock (FLAGS_KEY);
	sel = 1; // '1' corresponde al arkanoPi
	InicializaArkanoPi(&(juego.arkanoPi)); // Cambiamos el juego
}

// void SeleccionJuego1 (void): funcion encargada de seleccionar PongPi como
// modo de juego. Para ello tiene que inicializarlo y verificar que todos los flags
// correspondientes a las teclas estan a '0' para poder ver la pantalla inicial.
void SeleccionJuego2(void) {
	piLock (FLAGS_KEY); // Ponemos flags a '0' para evitar que se salte la pantalla inicial
	flags &= ~FLAG_SEL_2; // del estado WAIT_START
	flags &= ~FLAG_RAQUETA_IZQUIERDA;
	flags &= ~FLAG_RAQUETA_DERECHA;
	flags &= ~FLAG_RAQUETA_IZQUIERDA_1;
	flags &= ~FLAG_RAQUETA_DERECHA_1;
	flags &= ~FLAG_RAQUETA_IZQUIERDA_2;
	flags &= ~FLAG_RAQUETA_DERECHA_2;
	piUnlock (FLAGS_KEY);
	sel = 2; // '2' corresponde al pongPi
	InicializaPongPi(&(juego.pongPi)); // Cambiamos el juego
}

// void PausaJuego(void): funcion encargada de pausar y reanudar el juego.
// Lo que hace en realidad es poner a '0' todos los flags para reanudar correctamente
//la partida.
void PausaJuego(void) {
	piLock (FLAGS_KEY); // Ponemos casi todos los flags a '0'
	flags &= ~FLAG_PELOTA_ARKANOPI;
	flags &= ~FLAG_PELOTA_PONGPI;
	flags &= ~FLAG_RAQUETA_JOYSTICK;
	flags &= ~FLAG_RAQUETA_IZQUIERDA;
	flags &= ~FLAG_RAQUETA_DERECHA;
	flags &= ~FLAG_RAQUETA_IZQUIERDA_1;
	flags &= ~FLAG_RAQUETA_DERECHA_1;
	flags &= ~FLAG_RAQUETA_IZQUIERDA_2;
	flags &= ~FLAG_RAQUETA_DERECHA_2;
	flags &= ~FLAG_TECLADO_MATRICIAL;
	flags &= ~FLAG_SEL_1;
	flags &= ~FLAG_SEL_2;
	flags &= ~FLAG_CAMBIO_DIFICULTAD;
	piUnlock (FLAGS_KEY);
}

// void InicializaJuego (void): funcion encargada de llevar a cabo
// la oportuna inicializacon de toda variable o estructura de datos
// que resulte necesaria para el desarrollo del juego.
void InicializaJuego(void) {
	piLock (FLAGS_KEY); // Ponemos todos los flags a '0'
	flags &= ~FLAG_FINAL_JUEGO;
	flags &= ~FLAG_PELOTA_ARKANOPI;
	flags &= ~FLAG_PELOTA_PONGPI;
	flags &= ~FLAG_RAQUETA_JOYSTICK;
	flags &= ~FLAG_RAQUETA_IZQUIERDA;
	flags &= ~FLAG_RAQUETA_DERECHA;
	flags &= ~FLAG_RAQUETA_IZQUIERDA_1;
	flags &= ~FLAG_RAQUETA_DERECHA_1;
	flags &= ~FLAG_RAQUETA_IZQUIERDA_2;
	flags &= ~FLAG_RAQUETA_DERECHA_2;
	flags &= ~FLAG_TECLADO_MATRICIAL;
	flags &= ~FLAG_SEL_1;
	flags &= ~FLAG_SEL_2;
	flags &= ~FLAG_CAMBIO_DIFICULTAD;
	piUnlock (FLAGS_KEY);
	tmr_PELOTA = tmr_new(timer_isr_pelota); // Creamos contador de la pelota
	if( dif == 0 ){ // Modo dificultad facil
			tmr_startms(tmr_PELOTA, DIFICULTAD_FACIL);
		}
		else if( dif == 1 ){ // Modo dificultad medio
			tmr_startms(tmr_PELOTA, DIFICULTAD_MEDIA);
		}
		else if( dif == 2 ){ // Modo dificultad dificil
			tmr_startms(tmr_PELOTA, DIFICULTAD_DIFICIL);
		}
		else {// Trazas para depurar código en caso de error
			printf("Error en velocidad inicializa juego\n");
		}
	if ( sel==1 ) { // Variable de seleccion a 1 --> pintamos arkanoPi
		ActualizaPantalla(&(juego.arkanoPi));
#ifndef __MODO_ARKANOPI_NORMAL__
		tmr_JOYSTICK = tmr_new(timer_isr_joystick); // Creamos contador joystick
		tmr_startms(tmr_JOYSTICK, TIEMPO_MUESTRA); // Preparamos muestreo de voltaje de entrada
#endif
#ifdef __MODO_DEBUG_TERMINAL__
		piLock(STD_IO_BUFFER_KEY);
		PintaPantallaPorTerminal(&(juego.arkanoPi.pantalla));
		piUnlock(STD_IO_BUFFER_KEY);
#endif
	}
	else if ( sel == 2 ) { // Variable de seleccion a 2 --> pintamos pongPi
		ActualizaPantallaPongPi(&(juego.pongPi));
#ifdef __MODO_DEBUG_TERMINAL__
		piLock(STD_IO_BUFFER_KEY);
		PintaPantallaPorTerminalPongPi(&(juego.pongPi.pantalla));
		piUnlock(STD_IO_BUFFER_KEY);
#endif
	}
	else { // Trazas para depurar código en caso de error
		printf("Error inicializa juego \n");
	}
}

// void MueveRaqueta (void): funcion encargada del movimiento de la raqueta del
// ArkanoPi usando el joystick. Hace uso de otra funcion auxiliar detallada justo
// debajo.
void MueveRaqueta(void) {
	piLock (FLAGS_KEY);
	flags &= ~FLAG_RAQUETA_JOYSTICK;
	piUnlock (FLAGS_KEY);
	juego.arkanoPi.raqueta.x = CalculaPosicionPala(); // Calculamos posicion de la pala
	ActualizaPantalla(&(juego.arkanoPi));
#ifdef __MODO_DEBUG_TERMINAL__
	piLock(STD_IO_BUFFER_KEY);
	PintaPantallaPorTerminal(&(juego.arkanoPi.pantalla));
	piUnlock(STD_IO_BUFFER_KEY);
#endif
}

// int calculaPosicionPala (): funcion auxiliar encargada de calcular la posicion correspondiente
// a la raqueta dependiendo del movimiento del joystick (variacion de la tension de entrada).
int CalculaPosicionPala(){
	int result;
		if( voltaje_med >= 0 && voltaje_med < 0.3 ){
			result = 9;
		}
		else if( voltaje_med >= 0.3 && voltaje_med < 0.6 ){
			result = 8;
		}
		else if( voltaje_med >= 0.6 && voltaje_med < 0.9 ){
			result = 7;
		}
		else if( voltaje_med >= 0.9 && voltaje_med < 1.2 ){
			result = 6;
		}
		else if( voltaje_med >= 1.2 && voltaje_med <1.5 ){
			result = 5;
		}
		else if( voltaje_med >= 1.5 && voltaje_med < 1.8 ){
			result = 4;
		}
		else if( voltaje_med >= 1.8 && voltaje_med < 2.1 ){
			result = 3;
		}
		else if( voltaje_med >= 2.1 && voltaje_med < 2.4 ){
			result = 2;
		}
		else if( voltaje_med >= 2.4 && voltaje_med < 2.7 ){
			result = 1;
		}
		else if( voltaje_med >= 2.7 && voltaje_med < 3 ){
			result = 0;
		}
		else if( voltaje_med >= 3 && voltaje_med < 3.3 ){
			result = -1;
		}
		else if( voltaje_med >= 3.3 ){
			result = -2;
		}
		else {
			result = 3; // Raqueta al centro si hay error
		}
	return result;
}

#ifdef __MODO_ARKANOPI_NORMAL__
// void MueveRaquetaIzquierda (void): funcion encargada de ejecutar el movimiento hacia
// la izquierda contemplado para la raqueta del ArkanoPi con pulsadores.
void MueveRaquetaIzquierda(void) {
	piLock(FLAGS_KEY);
	flags &= ~FLAG_RAQUETA_IZQUIERDA;
	piUnlock(FLAGS_KEY);
	if( juego.arkanoPi.raqueta.x<9 ){
		juego.arkanoPi.raqueta.x+=1;
	}
	ActualizaPantalla(&(juego.arkanoPi));
#ifdef __MODO_DEBUG_TERMINAL__
	piLock(STD_IO_BUFFER_KEY);
	PintaPantallaPorTerminal(&(juego.arkanoPi.pantalla));
	piUnlock(STD_IO_BUFFER_KEY);
#endif
}

// void MueveRaquetaDerecha (void): funcion similar a la anterior
// encargada del movimiento hacia la derecha.
void MueveRaquetaDerecha(void) {
	piLock(FLAGS_KEY);
	flags &= ~FLAG_RAQUETA_DERECHA;
	piUnlock(FLAGS_KEY);
	if( juego.arkanoPi.raqueta.x>-2 ){
		juego.arkanoPi.raqueta.x -=1;
	}
	ActualizaPantalla(&(juego.arkanoPi));
#ifdef __MODO_DEBUG_TERMINAL__
	piLock(STD_IO_BUFFER_KEY);
	PintaPantallaPorTerminal(&(juego.arkanoPi.pantalla));
	piUnlock(STD_IO_BUFFER_KEY);
#endif
}
#endif

// void MueveRaquetaDerecha1 (void): funcion encargada de ejecutar el movimiento hacia
// la derecha contemplado para la raqueta del Jugador 1 (PongPi) con pulsadores.
void MueveRaquetaDerecha1(void) {
	piLock(FLAGS_KEY);
	flags &= ~FLAG_RAQUETA_DERECHA_1;
	piUnlock(FLAGS_KEY);
	if( juego.pongPi.raquetaDer.y>-2 ){ // Limite de la raqueta por la derecha
		juego.pongPi.raquetaDer.y-=1;
	}
	ActualizaPantallaPongPi(&(juego.pongPi));
#ifdef __MODO_DEBUG_TERMINAL__
	piLock(STD_IO_BUFFER_KEY);
	PintaPantallaPorTerminalPongPi(&(juego.pongPi.pantalla));
	piUnlock(STD_IO_BUFFER_KEY);
#endif
}

// void MueveRaquetaIzquierda1 (void): funcion similar a la anterior
// encargada del movimiento hacia la izquierda.
void MueveRaquetaIzquierda1(void) {
	piLock(FLAGS_KEY);
	flags &= ~FLAG_RAQUETA_IZQUIERDA_1;
	piUnlock(FLAGS_KEY);
	if( juego.pongPi.raquetaDer.y<6 ){ // Limite de la raqueta por la izquierda
		juego.pongPi.raquetaDer.y+=1;
	}
	ActualizaPantallaPongPi(&(juego.pongPi));
#ifdef __MODO_DEBUG_TERMINAL__
	piLock(STD_IO_BUFFER_KEY);
	PintaPantallaPorTerminalPongPi(&(juego.pongPi.pantalla));
	piUnlock(STD_IO_BUFFER_KEY);
#endif
}

// void MueveRaquetaDerecha2 (void): funcion encargada de ejecutar el movimiento hacia
// la derecha contemplado para la raqueta del Jugador 2 (PongPi) con pulsadores.
void MueveRaquetaDerecha2(void) {
	piLock(FLAGS_KEY);
	flags &= ~FLAG_RAQUETA_DERECHA_2;
	piUnlock(FLAGS_KEY);
	if( juego.pongPi.raquetaIzq.y<6 ){ // Limite de la raqueta por la derecha
		juego.pongPi.raquetaIzq.y+=1;
	}
	ActualizaPantallaPongPi(&(juego.pongPi));
#ifdef __MODO_DEBUG_TERMINAL__
	piLock(STD_IO_BUFFER_KEY);
	PintaPantallaPorTerminalPongPi(&(juego.pongPi.pantalla));
	piUnlock(STD_IO_BUFFER_KEY);
#endif
}

// void MueveRaquetaIzquierda2 (void): funcion similar a la anterior
// encargada del movimiento hacia la izquierda.
void MueveRaquetaIzquierda2(void) {
	piLock(FLAGS_KEY);
	flags &= ~FLAG_RAQUETA_IZQUIERDA_2;
	piUnlock(FLAGS_KEY);
	if( juego.pongPi.raquetaIzq.y>-2 ){ // Limite de la raqueta por la izquierda
		juego.pongPi.raquetaIzq.y-=1;
	}
	ActualizaPantallaPongPi(&(juego.pongPi));
#ifdef __MODO_DEBUG_TERMINAL__
	piLock(STD_IO_BUFFER_KEY);
	PintaPantallaPorTerminalPongPi(&(juego.pongPi.pantalla));
	piUnlock(STD_IO_BUFFER_KEY);
#endif
}

// void MovimientoPelota (void): funcion encargada de actualizar la
// posicion de la pelota del ArkanoPi conforme a la trayectoria definida para esta.
// Se utilizan dos funciones auxiliares adicionales que se definen
// justo debajo.
void MovimientoPelotaArkanoPi(void) {
	piLock(FLAGS_KEY);
	flags &= ~FLAG_PELOTA_ARKANOPI;
	piUnlock(FLAGS_KEY);
	if(juego.arkanoPi.pelota.yv == -1){
		switch(juego.arkanoPi.pelota.xv) {
		case(1):// Diagonal arriba izquierda
		if( HayLadrillo(juego.arkanoPi.pelota.x+1,juego.arkanoPi.pelota.y-1) == 1 ) { // Ladrillos
			EliminaLadrillo(juego.arkanoPi.pelota.x+1,juego.arkanoPi.pelota.y-1);
			juego.arkanoPi.pelota.yv*=-1;
			MovimientoPelotaArkanoPi();

		}
		else if( juego.arkanoPi.pelota.x == 9 ) { // Limite izquierda
			juego.arkanoPi.pelota.xv*=-1;
			MovimientoPelotaArkanoPi();
		}
		else if( juego.arkanoPi.pelota.y == 0 ) { // Limite superior
			juego.arkanoPi.pelota.yv*=-1;
			MovimientoPelotaArkanoPi();
		}
		else { // Ningun obstaculo
			juego.arkanoPi.pelota.x++;
			juego.arkanoPi.pelota.y--;
		}
		break;
		case(0):// Vertical arriba
		if( HayLadrillo(juego.arkanoPi.pelota.x,juego.arkanoPi.pelota.y-1)==1 ) { // Ladrillos
			EliminaLadrillo(juego.arkanoPi.pelota.x,juego.arkanoPi.pelota.y-1);
			juego.arkanoPi.pelota.yv*=-1;
			MovimientoPelotaArkanoPi();
		}
		else if( juego.arkanoPi.pelota.y == 0 ) { // Limite superior
			juego.arkanoPi.pelota.yv*=-1;
			MovimientoPelotaArkanoPi();
		}
		else { // Ningun obstaculo
			juego.arkanoPi.pelota.y--;
		}
		break;
		case(-1):// Diagonal arriba derecha
		if( HayLadrillo(juego.arkanoPi.pelota.x-1,juego.arkanoPi.pelota.y-1)==1 ) { // Ladrillos
			EliminaLadrillo(juego.arkanoPi.pelota.x-1,juego.arkanoPi.pelota.y-1);
			juego.arkanoPi.pelota.yv*=-1;
			MovimientoPelotaArkanoPi();
		}
		else if( juego.arkanoPi.pelota.x == 0 ) { // Limite derecha
			juego.arkanoPi.pelota.xv*=-1;
			MovimientoPelotaArkanoPi();
		}
		else if( juego.arkanoPi.pelota.y == 0 ) { // Limite superior
			juego.arkanoPi.pelota.yv*=-1;
			MovimientoPelotaArkanoPi();
		}
		else { // Ningun obstaculo
			juego.arkanoPi.pelota.x--;
			juego.arkanoPi.pelota.y--;
		}
		break;
		}
	}

	else {
		switch(juego.arkanoPi.pelota.xv) {
		case(1):// Diagonal abajo izquierda
		if( juego.arkanoPi.pelota.x == 9 ) { // Limite izquierda
			juego.arkanoPi.pelota.xv*=-1;
			MovimientoPelotaArkanoPi();
		}
		else if( juego.arkanoPi.pelota.y == 6) { // Va a pasar limite inferior
			piLock(FLAGS_KEY);
			flags |= FLAG_FINAL_JUEGO;
			piUnlock(FLAGS_KEY);
			break;
		}
		else if( HayLadrillo(juego.arkanoPi.pelota.x+1,juego.arkanoPi.pelota.y+1)==1 ) { // Ladrillos
			EliminaLadrillo(juego.arkanoPi.pelota.x+1,juego.arkanoPi.pelota.y+1);
			juego.arkanoPi.pelota.yv*=-1;
			MovimientoPelotaArkanoPi();
		}
		else if( juego.arkanoPi.pelota.y+1 == 6 ) { // Toca raqueta
			if( juego.arkanoPi.pelota.x+1 == juego.arkanoPi.raqueta.x ) {// Extremo derecho raqueta
				juego.arkanoPi.pelota.yv = -1;
				juego.arkanoPi.pelota.xv = -1;
				MovimientoPelotaArkanoPi();
			}
			else if( juego.arkanoPi.pelota.x+1 == juego.arkanoPi.raqueta.x+1 ) {// Centro raqueta
				juego.arkanoPi.pelota.yv = -1;
				juego.arkanoPi.pelota.xv = 0;
				MovimientoPelotaArkanoPi();
			}
			else if( juego.arkanoPi.pelota.x+1 == juego.arkanoPi.raqueta.x+2 ) {// Extremo izquierdo raqueta
				juego.arkanoPi.pelota.yv = -1;
				juego.arkanoPi.pelota.xv = 1;
				MovimientoPelotaArkanoPi();
			}
			else {
				juego.arkanoPi.pelota.y++;
				juego.arkanoPi.pelota.x++;
				MovimientoPelotaArkanoPi();
			}
		}
		else { // Ningun obstaculo
			juego.arkanoPi.pelota.y++;
			juego.arkanoPi.pelota.x++;
		}
		break;
		case(0):// Vertical abajo
		if( juego.arkanoPi.pelota.y == 6) { // Va a pasar limite inferior
				piLock(FLAGS_KEY);
				flags |= FLAG_FINAL_JUEGO;
				piUnlock(FLAGS_KEY);
				break;
		}
		else if( juego.arkanoPi.pelota.y+1 == 6 ) {// Toca raqueta
			if( juego.arkanoPi.pelota.x == juego.arkanoPi.raqueta.x ) {// Extremo derecho raqueta
				juego.arkanoPi.pelota.yv = -1;
				juego.arkanoPi.pelota.xv = -1;
				MovimientoPelotaArkanoPi();
			}
			else if( juego.arkanoPi.pelota.x == juego.arkanoPi.raqueta.x+1 ) {// Centro raqueta
				juego.arkanoPi.pelota.yv *= -1;
				MovimientoPelotaArkanoPi();
			}
			else if( juego.arkanoPi.pelota.x == juego.arkanoPi.raqueta.x+2 ) {// Extremo izquierdo raqueta
				juego.arkanoPi.pelota.yv = -1;
				juego.arkanoPi.pelota.xv = 1;
				MovimientoPelotaArkanoPi();
			}
			else {
				juego.arkanoPi.pelota.y++;
				MovimientoPelotaArkanoPi();
			}
		}
		else if( HayLadrillo(juego.arkanoPi.pelota.x,juego.arkanoPi.pelota.y+1)==1 ) { // Ladrillos
			EliminaLadrillo(juego.arkanoPi.pelota.x,juego.arkanoPi.pelota.y+1);
			juego.arkanoPi.pelota.yv*=-1;
			MovimientoPelotaArkanoPi();
		}
		else { // Ningun obstaculo
			juego.arkanoPi.pelota.y++;
		}
		break;
		case(-1):// Diagonal abajo derecha
		if( juego.arkanoPi.pelota.x == 0 ) { // Limite derecha
			juego.arkanoPi.pelota.xv*=-1;
			MovimientoPelotaArkanoPi();
		}
		else if( juego.arkanoPi.pelota.y == 6) { // Va a pasar limite inferior
			piLock(FLAGS_KEY);
			flags |= FLAG_FINAL_JUEGO;
			piUnlock(FLAGS_KEY);
			break;
		}
		else if( juego.arkanoPi.pelota.y+1 == 6 ) { // Toca raqueta
			if( juego.arkanoPi.pelota.x-1 == juego.arkanoPi.raqueta.x ) {// Extremo derecho raqueta
				juego.arkanoPi.pelota.yv = -1;
				juego.arkanoPi.pelota.xv = -1;
				MovimientoPelotaArkanoPi();
			}
			else if( juego.arkanoPi.pelota.x-1 == juego.arkanoPi.raqueta.x+1 ) {// Centro raqueta
				juego.arkanoPi.pelota.yv = -1;
				juego.arkanoPi.pelota.xv = 0;
				MovimientoPelotaArkanoPi();
			}
			else if( juego.arkanoPi.pelota.x-1 == juego.arkanoPi.raqueta.x+2 ) {// Extremo izquierdo raqueta
				juego.arkanoPi.pelota.yv = -1;
				juego.arkanoPi.pelota.xv = 1;
				MovimientoPelotaArkanoPi();
			}
			else {
				juego.arkanoPi.pelota.y++;
				juego.arkanoPi.pelota.x--;
				MovimientoPelotaArkanoPi();
			}
		}
		else if( HayLadrillo(juego.arkanoPi.pelota.x-1,juego.arkanoPi.pelota.y+1)==1 ) { // Ladrillos
			EliminaLadrillo(juego.arkanoPi.pelota.x-1,juego.arkanoPi.pelota.y+1);
			juego.arkanoPi.pelota.yv*=-1;
			MovimientoPelotaArkanoPi();
		}
		else { // Ningun obstaculo
			juego.arkanoPi.pelota.y++;
			juego.arkanoPi.pelota.x--;
		}
		break;
		}
	}
	ActualizaPantalla(&(juego.arkanoPi));
#ifdef __MODO_DEBUG_TERMINAL__
	piLock(STD_IO_BUFFER_KEY);
	PintaPantallaPorTerminal(&(juego.arkanoPi.pantalla));
	piUnlock(STD_IO_BUFFER_KEY);
#endif
	if(CalculaLadrillosRestantes((tipo_pantalla_arkanoPi*)(&(juego.arkanoPi.ladrillos))) == 0) { // Condicion fin de partida
		piLock(FLAGS_KEY);
		flags |= FLAG_FINAL_JUEGO;
		piUnlock(FLAGS_KEY);
	}
}

// int HayLadrillo(int x,int y): funcion auxiliar que se encarga de verificar
// si en una posicion pasada como argumento hay un ladrillo y en ese caso
// devuelve un '1'. En caso contrario devuelve un '0'.
int HayLadrillo(int x,int y) {
	if( juego.arkanoPi.ladrillos.matriz[x][y]== 1 ) {
		return 1;
	}
	return 0;
}

// void EliminaLadrillo(int x,int y): funcion auxiliar encargada de eliminar
// un ladrillo situado en la posicion pasada como argumento.
void EliminaLadrillo(int x,int y) {
	juego.arkanoPi.ladrillos.matriz[x][y] = 0;
}

// void MovimientoPelota (void): funcion encargada de actualizar la
// posicion de la pelota del PongPi conforme a la trayectoria definida para esta.
void MovimientoPelotaPongPi(void) {
	piLock(FLAGS_KEY);
	flags &= ~FLAG_PELOTA_PONGPI;
	piUnlock(FLAGS_KEY);
	if(juego.pongPi.pelota.xv == -1){
		switch(juego.pongPi.pelota.yv) {
		case(1): // Diagonal abajo derecha
					if( juego.pongPi.pelota.y == 6 ) { // Limite inferior
						juego.pongPi.pelota.yv*=-1;
						MovimientoPelotaPongPi();
					}
					else if( juego.pongPi.pelota.x-1 == 0 ) { // Toca raqueta Jugador 1
						if( juego.pongPi.pelota.y+1 == juego.pongPi.raquetaDer.y ) { // Extremo derecho raqueta
							juego.pongPi.pelota.yv = -1;
							juego.pongPi.pelota.xv = 1;
							MovimientoPelotaPongPi();
						}
						else if( juego.pongPi.pelota.y+1 == juego.pongPi.raquetaDer.y+1 ) { // Centro raqueta
							juego.pongPi.pelota.yv = 0;
							juego.pongPi.pelota.xv = 1;
							MovimientoPelotaPongPi();
						}
						else if( juego.pongPi.pelota.y+1 == juego.pongPi.raquetaDer.y+2 ) { // Extremo izquierdo raqueta
							juego.pongPi.pelota.yv = 1;
							juego.pongPi.pelota.xv = 1;
							MovimientoPelotaPongPi();
						}
						else { // Va a pasar limite derecho
							juego.pongPi.pelota.y++;
							juego.pongPi.pelota.x--;
						}
					}
					else { // Ningun obstaculo
						juego.pongPi.pelota.x--;
						juego.pongPi.pelota.y++;
					}
		break;

		case(0): // Horizontal derecha
					if( juego.pongPi.pelota.x-1 == 0 ) { // Toca raqueta Jugador 1
						if( juego.pongPi.pelota.y == juego.pongPi.raquetaDer.y ) { // Extremo derecho raqueta
							juego.pongPi.pelota.yv = -1;
							juego.pongPi.pelota.xv = 1;
							MovimientoPelotaPongPi();
						}
						else if( juego.pongPi.pelota.y == juego.pongPi.raquetaDer.y+1 ) { // Centro raqueta
							juego.pongPi.pelota.yv = 0;
							juego.pongPi.pelota.xv = 1;
							MovimientoPelotaPongPi();
						}
						else if( juego.pongPi.pelota.y == juego.pongPi.raquetaDer.y+2 ) { // Extremo izquierdo raqueta
							juego.pongPi.pelota.yv = 1;
							juego.pongPi.pelota.xv = 1;
							MovimientoPelotaPongPi();
						}
						else { // Va a pasar limite derecho
							juego.pongPi.pelota.x--;
						}
					}
					else { // Ningun obstaculo
						juego.pongPi.pelota.x--;
					}
		break;

		case(-1):// Diagonal arriba derecha
					if( juego.pongPi.pelota.y == 0 ) { // Limite superior
						juego.pongPi.pelota.yv*=-1;
						MovimientoPelotaPongPi();
					}
					else if( juego.pongPi.pelota.x-1 == 0 ) { // Toca raqueta Jugador 1
						if( juego.pongPi.pelota.y-1 == juego.pongPi.raquetaDer.y ) { // Extremo derecho raqueta
							juego.pongPi.pelota.yv = -1;
							juego.pongPi.pelota.xv = 1;
							MovimientoPelotaPongPi();
						}
						else if( juego.pongPi.pelota.y-1 == juego.pongPi.raquetaDer.y+1 ) { // Centro raqueta
							juego.pongPi.pelota.yv = 0;
							juego.pongPi.pelota.xv = 1;
							MovimientoPelotaPongPi();
						}
						else if( juego.pongPi.pelota.y-1 == juego.pongPi.raquetaDer.y+2 ) { // Extremo izquierdo raqueta
							juego.pongPi.pelota.yv = 1;
							juego.pongPi.pelota.xv = 1;
							MovimientoPelotaPongPi();
						}
						else { // Va a pasar limite derecho
							juego.pongPi.pelota.x--;
							juego.pongPi.pelota.y--;
						}
					}
					else { // Ningun obstaculo
						juego.pongPi.pelota.x--;
						juego.pongPi.pelota.y--;
					}
		break;
		}
	}

	else {
		switch(juego.pongPi.pelota.yv) {
		case(1):// Diagonal abajo izquierda
							if( juego.pongPi.pelota.y == 6 ) { // Limite inferior
								juego.pongPi.pelota.yv*=-1;
								MovimientoPelotaPongPi();
							}
							else if( juego.pongPi.pelota.x+1 == 9 ) { // Toca raqueta Jugador 2
								if( juego.pongPi.pelota.y+1 == juego.pongPi.raquetaIzq.y ) { // Extremo derecho raqueta
									juego.pongPi.pelota.yv = -1;
									juego.pongPi.pelota.xv = -1;
									MovimientoPelotaPongPi();
								}
								else if( juego.pongPi.pelota.y+1 == juego.pongPi.raquetaIzq.y+1 ) { // Centro raqueta
									juego.pongPi.pelota.yv = 0;
									juego.pongPi.pelota.xv = -1;
									MovimientoPelotaPongPi();
								}
								else if( juego.pongPi.pelota.y+1 == juego.pongPi.raquetaIzq.y+2 ) { // Extremo izquierdo raqueta
									juego.pongPi.pelota.yv = 1;
									juego.pongPi.pelota.xv = -1;
									MovimientoPelotaPongPi();
								}
								else { // Va a pasar limite izquierdo
									juego.pongPi.pelota.y++;
									juego.pongPi.pelota.x++;
								}
							}
							else { // Ningun obstaculo
								juego.pongPi.pelota.x++;
								juego.pongPi.pelota.y++;
							}
		break;

		case(0):// Horizontal izquierda
					if( juego.pongPi.pelota.x+1 == 9 ) { // Toca raqueta Jugador 2
						if( juego.pongPi.pelota.y == juego.pongPi.raquetaIzq.y ) { // Extremo derecho raqueta
							juego.pongPi.pelota.yv = -1;
							juego.pongPi.pelota.xv = -1;
							MovimientoPelotaPongPi();
						}
						else if( juego.pongPi.pelota.y == juego.pongPi.raquetaIzq.y+1 ) { // Centro raqueta
							juego.pongPi.pelota.yv = 0;
							juego.pongPi.pelota.xv = -1;
							MovimientoPelotaPongPi();
						}
						else if( juego.pongPi.pelota.y == juego.pongPi.raquetaIzq.y+2 ) { // Extremo izquierdo raqueta
							juego.pongPi.pelota.yv = +1;
							juego.pongPi.pelota.xv = -1;
							MovimientoPelotaPongPi();
						}
						else { // Va a pasar limite derecho
							juego.pongPi.pelota.x++;
						}
					}
					else { // Ningun obstaculo
						juego.pongPi.pelota.x++;
					}
		break;

		case(-1):// Diagonal arriba izquierda
					if( juego.pongPi.pelota.y == 0 ) { // Limite superior
						juego.pongPi.pelota.yv*=-1;
						MovimientoPelotaPongPi();
					}
					else if( juego.pongPi.pelota.x+1 == 9 ) { // Toca raqueta Jugador 1
						if( juego.pongPi.pelota.y-1 == juego.pongPi.raquetaDer.y ) { // Extremo derecho raqueta
							juego.pongPi.pelota.yv = -1;
							juego.pongPi.pelota.xv = -1;
							MovimientoPelotaPongPi();
						}
						else if( juego.pongPi.pelota.y-1 == juego.pongPi.raquetaDer.y+1 ) { // Centro raqueta
							juego.pongPi.pelota.yv = 0;
							juego.pongPi.pelota.xv = -1;
							MovimientoPelotaPongPi();
						}
						else if( juego.pongPi.pelota.y-1 == juego.pongPi.raquetaDer.y+2 ) { // Extremo izquierdo raqueta
							juego.pongPi.pelota.yv = +1;
							juego.pongPi.pelota.xv = -1;
							MovimientoPelotaPongPi();
						}
						else { // Va a pasar limite izquierdo
							juego.pongPi.pelota.x++;
							juego.pongPi.pelota.y--;
						}
					}
					else { // Ningun obstaculo
						juego.pongPi.pelota.x++;
						juego.pongPi.pelota.y--;
					}
		break;
		}
	}
	ActualizaPantallaPongPi(&(juego.pongPi));
#ifdef __MODO_DEBUG_TERMINAL__
	piLock(STD_IO_BUFFER_KEY);
	PintaPantallaPorTerminalPongPi(&(juego.pongPi.pantalla));
	piUnlock(STD_IO_BUFFER_KEY);
#endif
	if( juego.pongPi.pelota.x == 0 ) { // Condicion gana jugador 2
		winner = 2;
		piLock(FLAGS_KEY);
		flags |= FLAG_FINAL_JUEGO;
		piUnlock(FLAGS_KEY);
	}
	else if( juego.pongPi.pelota.x == 9 ) { // Condicion gana jugador 1
		winner = 1;
		piLock(FLAGS_KEY);
		flags |= FLAG_FINAL_JUEGO;
		piUnlock(FLAGS_KEY);
	}
}

// void FinalJuego (void): funcion encargada de mostrar en la ventana de
// terminal los mensajes necesarios para informar acerca del resultado del juego.
void FinalJuego(void) {
	piLock(FLAGS_KEY);
	flags &= ~FLAG_FINAL_JUEGO;
	piUnlock(FLAGS_KEY);
	tmr_stop(tmr_PELOTA); // Finalizamos este timer
	if( sel == 1 ) {
#ifndef __MODO_ARKANOPI_NORMAL__
		tmr_stop(tmr_JOYSTICK); // Finalizamos timer para no consumir recursos
#endif
		int result = 20-CalculaLadrillosRestantes((tipo_pantalla_arkanoPi*)(&(juego.arkanoPi.ladrillos)));
		printf("RESULTADO:\n %d\n",result);
	}
	else if( sel == 2 ){
		if( winner == 1 ){
			printf("Gana jugador 1 \n");
		}
		else if( winner == 2 ){
			printf("Gana jugador 2 \n");
		}
	}
	else { // Trazas para depurar código en caso de error
		printf("Error seleccion final juego \n");
	}
}

// void ReseteaJuego (void): funcion encargada de llevar a cabo la
// reinicializacionn de cuantas variables o estructuras resulten
// necesarias para dar comienzo a una nueva partida.
void ReseteaJuego(void) {
	piLock(FLAGS_KEY); // Ponemos a '0' todos los flags
	flags &= ~FLAG_FINAL_JUEGO;
	flags &= ~FLAG_PELOTA_ARKANOPI;
	flags &= ~FLAG_PELOTA_PONGPI;
	flags &= ~FLAG_RAQUETA_JOYSTICK;
	flags &= ~FLAG_RAQUETA_IZQUIERDA;
	flags &= ~FLAG_RAQUETA_DERECHA;
	flags &= ~FLAG_RAQUETA_IZQUIERDA_1;
	flags &= ~FLAG_RAQUETA_DERECHA_1;
	flags &= ~FLAG_RAQUETA_IZQUIERDA_2;
	flags &= ~FLAG_RAQUETA_DERECHA_2;
	flags &= ~FLAG_TECLADO_MATRICIAL;
	flags &= ~FLAG_SEL_1;
	flags &= ~FLAG_SEL_2;
	flags &= ~FLAG_CAMBIO_DIFICULTAD;
	piUnlock(FLAGS_KEY);
	if( sel == 1 ){ // Reseteamos arkanoPi
		InicializaArkanoPi(&(juego.arkanoPi));
	}
	else if( sel == 2 ){ // Reseteamos arkanoPi
		InicializaPongPi(&(juego.pongPi));
		if( winner == 2 ){ // Empieza el que pierde, el caso de ganador jugador 1 esta por defecto
			juego.pongPi.pelota.xv = -1;
		}
	}
	else { // Trazas para depurar código en caso de error
		printf("Error en resetea juego \n");
	}
}

// void PreparaCambio (void): funcion encargada de preparar el cambio de juego. Para
// ello tiene que poner a '0' todos los flags y modificar el selector de juego para que
// aparezca la pantalla de bienvenida.
void PreparaCambio(void) {
	piLock(FLAGS_KEY);
	flags &= ~FLAG_FINAL_JUEGO; // Ponemos a '0' todos los flags
	flags &= ~FLAG_PELOTA_ARKANOPI;
	flags &= ~FLAG_PELOTA_PONGPI;
	flags &= ~FLAG_RAQUETA_JOYSTICK;
	flags &= ~FLAG_RAQUETA_IZQUIERDA;
	flags &= ~FLAG_RAQUETA_DERECHA;
	flags &= ~FLAG_RAQUETA_IZQUIERDA_1;
	flags &= ~FLAG_RAQUETA_DERECHA_1;
	flags &= ~FLAG_RAQUETA_IZQUIERDA_2;
	flags &= ~FLAG_RAQUETA_DERECHA_2;
	flags &= ~FLAG_TECLADO_MATRICIAL;
	flags &= ~FLAG_SEL_1;
	flags &= ~FLAG_SEL_2;
	flags &= ~FLAG_CAMBIO_DIFICULTAD;
	piUnlock(FLAGS_KEY);
	sel = 0; // Seleccion en 0 ya que queremos que salga la pantalla del menu
	dif = 0; // Ponemos dificultad a modo normal
}

// void CambioDificultad(void): funcion encargada de cambiar la dificultad de juego.
// Esto equivale a modificar el tiempo del timer que mueve la pelota.
void CambioDificultad(void) {
	piLock(FLAGS_KEY);
	flags &= ~FLAG_CAMBIO_DIFICULTAD;
	flags &= ~FLAG_PELOTA_ARKANOPI;
	flags &= ~FLAG_PELOTA_PONGPI;
	piUnlock(FLAGS_KEY);
	if( dif < 2) { // Llevamos la cuenta del contador
		dif++;
	}
	else {
		dif=0;
	}
	if( dif == 0 ){ // Modo dificultad normal
		printf("Modo facil \n");
		tmr_startms(tmr_PELOTA, DIFICULTAD_FACIL);
	}
	else if( dif == 1 ){ // Modo dificultad medio
		printf("Modo medio \n");
		tmr_startms(tmr_PELOTA, DIFICULTAD_MEDIA);
	}
	else if( dif == 2 ){ // Modo dificultad dificil
		printf("Modo dificil \n");
		tmr_startms(tmr_PELOTA, DIFICULTAD_DIFICIL);
	}
	else {// Trazas para depurar código en caso de error
		printf("Error en cambio de velocidad\n");
	}
}

//------------------------------------------------------
// FUNCIONES DE MEDIDA
//------------------------------------------------------

// float lectura_ADC (void): funcion encargada de leer el valor del ADC en tension.
// Se usa cada vez que salta el temporizador del joystick para analizar la posicion
// correcta de la raqueta del ArkanoPi.
float lectura_ADC(void) {
	unsigned char ByteSPI[3]; // Buffer lectura escritura SPI
	int resultado_SPI = 0; // Control operacion SPI
	float voltaje_medido = 0.0; // Valor medido, a calcular a partir del buffer
	   ByteSPI[0] = 0b10011111; // Configuracion ADC (10011111 unipolar, 0-2.5v,canal 0, salida 1), bipolar 0b10010111
       ByteSPI[1] = 0b0;
       ByteSPI[2] = 0b0;
	resultado_SPI = wiringPiSPIDataRW (SPI_ADC_CH, ByteSPI, 3); // Enviamos y leemos tres bytes (8+12+4 bits)
	usleep(20); // Se para 20 us
	int salida_SPI = ((ByteSPI[1] << 6) | (ByteSPI[2] >> 2)) & 0xFFF;
       /*Caso unipolar */
	voltaje_medido = 2*2.50 * (((float) salida_SPI)/4095.0);
	#ifdef VERBOSE
	printf("Lectura ADC MAX1246: %d\n", resultado_SPI);
	printf("Primer byte: %02X \n", ByteSPI[0]);
	printf("Segundo Byte: %02X \n", ByteSPI[1]);
	printf("Tercer byte: %02X \n", ByteSPI[2]);
	printf("Valor entero: %i \n", salida_SPI);
	printf("Voltaje medido: %f \n",voltaje_medido);
	fflush(stdout);
	#endif
	return voltaje_medido;
	}

//------------------------------------------------------
// FUNCIONES DE INICIALIZACION
//------------------------------------------------------

// int systemSetup (void): procedimiento de configuracion del sistema.
// Realiza todas las operaciones necesarias para:
// configurar el uso de librerias (e.g. Wiring Pi),
// configurar las interrupciones externas asociadas a los pines GPIO,
// configurar las interrupciones periodicas y sus correspondientes temporizadores,
// crear los threads adicionales que pueda requerir el sistema.
int systemSetup(void) {
	if(wiringPiSetupGpio() < 0) { // Configuramos la librería de wiringPi para poder utilizarla
		printf("Unable to setup wiringPi \n");
		return -1;
	}
	if (wiringPiSPISetup (SPI_ADC_CH, SPI_ADC_FREQ) < 0) { // Configuramos el SPI para usar el ADC con la conexion del canal 0 a 1 MHz
		printf ("No se pudo inicializar el dispositivo SPI (CH 0) \n");
		exit (1);
		return -2;
	}

	int input,output;
	for( input = 0 ; input < 7 ; input++ ) { // Configuramos todos los pines de entrada para poder usarlos
		pinMode(gpio_input[input],INPUT);
		if ( input != 0 ) { // Pin fila teclado no necesita
			pullUpDnControl(gpio_input[input], PUD_DOWN); // Resistencia interna de pull down
		}
	}
	for( output = 0 ; output < 13 ; output++ ) { // Configuramos todos los pines de salida para poder usarlos
		pinMode(gpio_output[output],OUTPUT);
	}

	wiringPiISR(PULSADOR_RAQUETA_J1_DER, INT_EDGE_FALLING,boton_isr_right_1);// Definimos los pulsadores utilizando los pines correspondientes (#define)
	wiringPiISR(PULSADOR_RAQUETA_J1_IZQ, INT_EDGE_FALLING,boton_isr_left_1);
	wiringPiISR(PULSADOR_RAQUETA_J2_DER, INT_EDGE_FALLING,boton_isr_right_2);
	wiringPiISR(PULSADOR_RAQUETA_J2_IZQ, INT_EDGE_FALLING,boton_isr_left_2);
	wiringPiISR(PULSADOR_CAMBIO_DIF, INT_EDGE_FALLING,boton_isr_dif);
#ifndef __MODO_ARKANOPI_NORMAL__
	wiringPiISR(PULSADOR_QUIT, INT_EDGE_FALLING,boton_isr_quit);
#endif

#ifdef __MODO_ARKANOPI_NORMAL__
	pinMode(PULSADOR_RAQUETA_DER,INPUT);
	pinMode(PULSADOR_RAQUETA_IZQ,INPUT);
	pullUpDnControl(PULSADOR_RAQUETA_DER, PUD_DOWN);
	pullUpDnControl(PULSADOR_RAQUETA_IZQ, PUD_DOWN);
	wiringPiISR(PULSADOR_RAQUETA_DER,INT_EDGE_FALLING,boton_isr_right);
	wiringPiISR(PULSADOR_RAQUETA_IZQ,INT_EDGE_FALLING,boton_isr_left);
#endif

	int x,y;
	x = piThreadCreate(thread_teclado_matricial); // Iniciamos threads que utilizamos posteriormente
	y = piThreadCreate(thread_leds);

	if(x!=0) { // Verificamos que no se hayan producido errores
		printf("it didn't start! (y) \n");
		return -3;
	}
	if(y!=0) { // Verificamos que no se hayan producido errores
		printf("it didn't start! (y) \n");
		return -4;
	}
#ifdef __MODO_DEBUG_TECLADO__
	int z;
	z = piThreadCreate(thread_explora_teclado);
	if(z!=0) { // Verificamos que no se hayan producido errores
		printf("it didn't start! (y) \n");
		return -5;
	}
#endif

	return 1;
}

// void fsm_setup (fsm_t* fsm): configuracion inicial de la maquina de estados.
void fsm_setup(fsm_t* fsm){
	piLock(FLAGS_KEY);
	flags = 0; // Inicializamos la variable que nos permite controlar la concurrencia de hebras
	piUnlock(FLAGS_KEY);
} 

//------------------------------------------------------
// FUNCIONES ILUMINADO DE LEDS
//------------------------------------------------------

// void Filas(): Método que realiza el barrido de la pantalla en uso para
// determinar si hay que encender un led.
void Filas(){
	int j;
#ifdef __MODO_LEDS_NORMAL__
	for( j = 0 ; j < 7 ; j++ ){
		if ( sel == 0 ) { // Pantalla inicial de bienvenida del menu de juego
			if( matrizBienvenida[9-contador_col][j]==1 ){
							digitalWrite(leds_fila[j],LOW);
						}
		}
		else if ( sel == 1 ) { // Pantalla del juego 1 (ArkanoPi)
			if( juego.arkanoPi.pantalla.matriz[9-contador_col][j]==1 ){
				digitalWrite(leds_fila[j],LOW);
			}
		}
		else if ( sel == 2 ) { // Pantalla del juego 2 (PongPi)
			if( juego.pongPi.pantalla.matriz[9-contador_col][j]==1 ){
				digitalWrite(leds_fila[j],LOW);
			}
		}
		else { // Trazas para depurar código en caso de error
			printf("Error ilumina leds \n");
		}
	}
}
#endif

#ifndef __MODO_LEDS_NORMAL__
	// Empieza envio de trama
	for( j = 0; j < 7; j++ ){
		if( sel == 0 ) {
			if( matrizBienvenida[9-contador_col][j] == 1 ){
				digitalWrite(SHCP,LOW); // Creamos nuestro proprio clk para pasar trama al registro de desplazamiento (flancos subida)
				digitalWrite(DS,LOW); // enviamos tramas de 124us y clk con ese mismo periodo
				delayMicroseconds(62);
				digitalWrite(SHCP,HIGH);
				delayMicroseconds(62);
			}
			else {
				digitalWrite(SHCP,LOW); // Creamos nuestro proprio clk para pasar trama al registro de desplazamiento (flancos subida)
				digitalWrite(DS,HIGH); // enviamos tramas de 124us y clk con ese mismo periodo
				delayMicroseconds(62);
				digitalWrite(SHCP,HIGH);
				delayMicroseconds(62);
			}
		}
		else if( sel == 1 ){
			if( juego.arkanoPi.pantalla.matriz[9-contador_col][j] == 1 ){
				digitalWrite(SHCP,LOW); // Creamos nuestro proprio clk para pasar trama al registro de desplazamiento (flancos subida)
				digitalWrite(DS,LOW); // enviamos tramas de 124us y clk con ese mismo periodo
				delayMicroseconds(62);
				digitalWrite(SHCP,HIGH);
				delayMicroseconds(62);
			}
			else {
				digitalWrite(SHCP,LOW); // Creamos nuestro proprio clk para pasar trama al registro de desplazamiento (flancos subida)
				digitalWrite(DS,HIGH); // enviamos tramas de 124us y clk con ese mismo periodo
				delayMicroseconds(62);
				digitalWrite(SHCP,HIGH);
				delayMicroseconds(62);
			}
		}
		else if( sel == 2){
			if( juego.pongPi.pantalla.matriz[9-contador_col][j] == 1 ){
				digitalWrite(SHCP,LOW); // Creamos nuestro proprio clk para pasar trama al registro de desplazamiento (flancos subida)
				digitalWrite(DS,LOW); // enviamos tramas de 124us y clk con ese mismo periodo
				delayMicroseconds(62);
				digitalWrite(SHCP,HIGH);
				delayMicroseconds(62);
			}
			else {
				digitalWrite(SHCP,LOW); // Creamos nuestro proprio clk para pasar trama al registro de desplazamiento (flancos subida)
				digitalWrite(DS,HIGH); // enviamos tramas de 124us y clk con ese mismo periodo
				delayMicroseconds(62);
				digitalWrite(SHCP,HIGH);
				delayMicroseconds(62);
			}
		}
		else{
			printf("Error seleccion refresco leds\n");
		}
	}
	digitalWrite(SHCP,LOW); // Bit 8 de la trama ponemos '1' para depurar en osciloscopio
	digitalWrite(DS,HIGH); // No se usara posteriormente
	delayMicroseconds(62);
	digitalWrite(SHCP,LOW);
	delayMicroseconds(62);
	digitalWrite(DS,LOW);
	digitalWrite(SHCP,LOW);
	// fin envio de trama
#endif
}

// void EnciendeLeds(): Método que excita las columnas en función de sus salidas correspondiente
// del decodificador 4 a 16.
void EnciendeLeds(){ // Ponemos a '1' las columnas y a '0' las filas para encender un led
	int i;
#ifdef __MODO_LEDS_NORMAL__
	digitalWrite(2,HIGH); // FILAS
	digitalWrite(3,HIGH);
	digitalWrite(4,HIGH);
	digitalWrite(7,HIGH);
	digitalWrite(23,HIGH);
	digitalWrite(24,HIGH);
	digitalWrite(25,HIGH);
	digitalWrite(14,LOW); // COLUMNAS
	digitalWrite(17,LOW);
	digitalWrite(18,LOW);
	digitalWrite(22,LOW);
#endif
	for( i = 0 ; i < 4 ; i++ ) {
		digitalWrite( leds_columna[i],( contador_col >> i ) & 1 ); // Seguimos tabla del decodificador 4 a 16
	}
	Filas(); // Encendemos leds si hay un 1 en la pantalla actual

	if( contador_col < 9 ){ // El contador sirve para pasar por cada columna
		contador_col++;
	}
	else{
		contador_col = 0;
	}
}

//------------------------------------------------------
// PI THREADs
//------------------------------------------------------

// PI_THREAD(thread_teclado_matricial): Realiza la lectura del teclado matricial.
// En este caso solo se lee la primera fila ya que el resto del teclado no nos interesa.
PI_THREAD(thread_teclado_matricial) {
	int fila = 0,columna;
	while(1) {
		for( columna = 0; columna < 2; columna++ ) { // Excitamos columnas una a una
			digitalWrite(columna,HIGH); // Enviamos un '1' a la columna
			delay(100); // Esperamos respuesta octoaopladores
			if( digitalRead (FILA1_TECLADO_MATRICIAL)== 1 ) { // Vemos si esta pulsada
					while( digitalRead (FILA1_TECLADO_MATRICIAL)== 1 ){ // Esperamos a soltar tecla
						delay(100);
					}
					piLock(FLAGS_KEY);
					flags |= FLAG_TECLADO_MATRICIAL;
					piUnlock(FLAGS_KEY);

					if( fila == 0 && columna == 0) { // Tecla "1" pulsada
						piLock(FLAGS_KEY);
						flags |= FLAG_SEL_1;
						piUnlock(FLAGS_KEY);
					}

					else if( fila == 0 && columna == 1) { // Tecla "2" pulsada
						piLock(FLAGS_KEY);
						flags |= FLAG_SEL_2;
						piUnlock(FLAGS_KEY);
					}
				}
				digitalWrite(columna,LOW); // Enviamos '0' a la columna para que no quede en '1'
			}
		}
	}

// PI_THREAD(thread_leds): Se encarga de hacer el barrido de los leds cada
// 1ms.
PI_THREAD(thread_leds) {
	while(1){
		EnciendeLeds();
#ifdef __MODO_LEDS_NORMAL__
		delay(1);
#endif
		}
}

// PI_THREAD(thread_explora_teclado): Explora el teclado del PC para jugar utilizando
// las teclas. Solo se usa en caso de problemas con el resto de hardware.
PI_THREAD(thread_explora_teclado) {
	int teclaPulsada;
	while (1) {
		delay(10); // wiringPi pauses program
		if(kbhit()) { // Funcion que detecta si se ha producido pulsacion de tecla alguna
			teclaPulsada = kbread(); // Funcion que devuelve la tecla pulsada

			switch(teclaPulsada) {
			case 'i': // Desplazamiento de la raqueta ArkanoPi hacia la izquierda
				piLock(FLAGS_KEY);
				flags |= FLAG_RAQUETA_IZQUIERDA;
				piUnlock(FLAGS_KEY);
				break;

			case 'o': // Desplazamiento de la raqueta ArkanoPi hacia la derecha
				piLock(FLAGS_KEY);
				flags |= FLAG_RAQUETA_DERECHA;
				piUnlock(FLAGS_KEY);
				break;

			case 'y': // Desplazamiento de la raqueta PongPi jugador1 hacia la izquierda
				piLock(FLAGS_KEY);
				flags |= FLAG_RAQUETA_IZQUIERDA_1;
				piUnlock(FLAGS_KEY);
				break;

			case 'u': // Desplazamiento de la raqueta PongPi jugador1 hacia la derecha
				piLock(FLAGS_KEY);
				flags |= FLAG_RAQUETA_DERECHA_1;
				piUnlock(FLAGS_KEY);
				break;

			case 'w': // Desplazamiento de la raqueta PongPi jugador2 hacia la izquierda
				piLock(FLAGS_KEY);
				flags |= FLAG_RAQUETA_IZQUIERDA_2;
				piUnlock(FLAGS_KEY);
				break;

			case 'e': // Desplazamiento de la raqueta PongPi jugador2 hacia la derecha
				piLock(FLAGS_KEY);
				flags |= FLAG_RAQUETA_DERECHA_2;
				piUnlock(FLAGS_KEY);
				break;

			case '1': // Seleccon juego 1
				piLock(FLAGS_KEY);
				flags |= FLAG_SEL_1;
				piUnlock(FLAGS_KEY);
				break;

			case '2': // Seleccion juego 2
				piLock(FLAGS_KEY);
				flags |= FLAG_SEL_2;
				piUnlock(FLAGS_KEY);
				break;

			case 'p': // Actualiza posicion de la pelota
				if ( sel == 1 ){
					piLock(FLAGS_KEY);
					flags |= FLAG_PELOTA_ARKANOPI;
					piUnlock(FLAGS_KEY);
				}
				else if ( sel == 2 ){
					piLock(FLAGS_KEY);
					flags |= FLAG_PELOTA_PONGPI;
					piUnlock(FLAGS_KEY);
				}
				else {
					printf("Error tecla pelota \n");
					break;
			case 'q':
				exit(0);
				break;
				}
			}
		}
	}
}

int main () {
	unsigned int next;

	fsm_trans_t state_tabla[] = { // Maquina estados principal transiciones: {EstadoOrigen, FuncionEntrada, EstadoDestino, FuncionSalida}
			{WAIT_SEL,CompruebaSelJuego1,WAIT_START,SeleccionJuego1},
			{WAIT_SEL,CompruebaSelJuego2,WAIT_START,SeleccionJuego2},
			{WAIT_START,CompruebaTeclaPulsada,WAIT_PUSH,InicializaJuego},
			{WAIT_PUSH,CompruebaFinalJuego,WAIT_END,FinalJuego},
			{WAIT_PUSH,CompruebaPausa,WAIT_PAUSE,PausaJuego},
			{WAIT_PAUSE,CompruebaPausa,WAIT_PUSH,PausaJuego},
			{WAIT_PUSH,CompruebaPelotaArkano,WAIT_PUSH,MovimientoPelotaArkanoPi},
			{WAIT_PUSH,CompruebaPelotaPongPi,WAIT_PUSH,MovimientoPelotaPongPi},
#ifndef __MODO_ARKANOPI_NORMAL__
			{WAIT_PUSH,CompruebaRaqueta,WAIT_PUSH,MueveRaqueta},
#endif
#ifdef __MODO_ARKANOPI_NORMAL__
			{WAIT_PUSH,CompruebaRaquetaDerecha,WAIT_PUSH,MueveRaquetaDerecha},
			{WAIT_PUSH,CompruebaRaquetaIzquierda,WAIT_PUSH,MueveRaquetaIzquierda},
#endif
			{WAIT_PUSH,CompruebaRaquetaDerecha1,WAIT_PUSH,MueveRaquetaDerecha1},
			{WAIT_PUSH,CompruebaRaquetaIzquierda1,WAIT_PUSH,MueveRaquetaIzquierda1},
			{WAIT_PUSH,CompruebaRaquetaDerecha2,WAIT_PUSH,MueveRaquetaDerecha2},
			{WAIT_PUSH,CompruebaRaquetaIzquierda2,WAIT_PUSH,MueveRaquetaIzquierda2},
			{WAIT_PUSH,CompruebaCambioDificultad,WAIT_PUSH,CambioDificultad},
			{WAIT_END,CompruebaCambioJuego,WAIT_SEL,PreparaCambio},
			{WAIT_END,CompruebaTeclaPulsada,WAIT_START,ReseteaJuego},
			{-1,NULL,-1,NULL},
	};

	fsm_t* fsm = fsm_new (WAIT_SEL, state_tabla, NULL); // Configuración e inicialización del sistema.
	systemSetup();
	fsm_setup(fsm);
	next=millis();
	while(1){
		fsm_fire(fsm); // Cada 10ms verifica si hay algún flag activado.
		next+=CLK_MS;
		delay_until(next);
	}
	fsm_destroy(fsm);
}
