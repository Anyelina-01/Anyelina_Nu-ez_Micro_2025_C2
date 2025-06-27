#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define ESTADO_INICIAL 0
#define ESTADO_CERRANDO 1
#define ESTADO_ABRIENDO 2
#define ESTADO_CERRADO 3
#define ESTADO_ABIERTO 4
#define ESTADO_ERR 5
#define ESTADO_STOP 6

// Simulación del tiempo (para parpadeo)
void delay_ms(int ms) {
    // Simulación: En microcontrolador real usarías _delay_ms(ms) o similar
}

// Prototipos
int Func_ESTADO_INICIAL(void);
int Func_ESTADO_CERRANDO(void);
int Func_ESTADO_ABRIENDO(void);
int Func_ESTADO_CERRADO(void);
int Func_ESTADO_ABIERTO(void);
int Func_ESTADO_ERR(void);
int Func_ESTADO_STOP(void);

// Estados globales
int EstadoSiguiente = ESTADO_INICIAL;
int EstadoActual = ESTADO_INICIAL;
int EstadoAnterior = ESTADO_INICIAL;

// Entradas/Salidas
struct IO {
    unsigned int LSC:1;
    unsigned int LSA:1;
    unsigned int BA:1;
    unsigned int BC:1;
    unsigned int SE:1;
    unsigned int PP:1;  // Botón multifunción
    unsigned int MA:1;
    unsigned int MC:1;
    unsigned int Buzzer:1;
    unsigned int Lampara:1;
} io;

// Estado de la máquina
struct STATUS {
    unsigned int cntTimerCA;
    unsigned int cntRunTimer;
} status = {0, 0};

// Configuración
struct CONFIG {
    unsigned int RunTimer;
    unsigned int TimerCA;
} config = {180, 100};

// Variables adicionales
int codigoError = 0;

// Funciones auxiliares
void ApagarMotores() {
    io.MA = false;
    io.MC = false;
}

void ActualizarLampara(int estado) {
    switch (estado) {
        case ESTADO_ABRIENDO:
            io.Lampara = 1;
            delay_ms(500);
            io.Lampara = 0;
            delay_ms(500);
            break;
        case ESTADO_CERRANDO:
            io.Lampara = 1;
            delay_ms(250);
            io.Lampara = 0;
            delay_ms(250);
            break;
        case ESTADO_ABIERTO:
            io.Lampara = 1;
            break;
        default:
            io.Lampara = 0;
    }
}

int Func_ESTADO_INICIAL(void) {
    EstadoAnterior = EstadoActual;
    EstadoActual = ESTADO_INICIAL;

    if (io.SE) return ESTADO_STOP;
    if (io.LSC && io.LSA) return ESTADO_ERR;
    if (io.LSC && !io.LSA) return ESTADO_CERRADO;
    if (!io.LSC && io.LSA) return ESTADO_ABIERTO;
    if (!io.LSC && !io.LSA) return ESTADO_CERRANDO;

    return ESTADO_INICIAL;
}

int Func_ESTADO_CERRANDO(void) {
    EstadoAnterior = EstadoActual;
    EstadoActual = ESTADO_CERRANDO;
    status.cntRunTimer = 0;
    io.MA = false;
    io.MC = true;
    io.BC = false;

    for (;;) {
        ActualizarLampara(ESTADO_CERRANDO);
        status.cntRunTimer++;
        if (io.LSC) return ESTADO_CERRADO;
        if (status.cntRunTimer > config.RunTimer) {
            codigoError = 2;
            return ESTADO_ERR;
        }
        if (io.BA || io.PP || io.SE) return ESTADO_STOP;
    }
}

int Func_ESTADO_ABRIENDO(void) {
    EstadoAnterior = EstadoActual;
    EstadoActual = ESTADO_ABRIENDO;
    status.cntRunTimer = 0;
    io.MA = true;
    io.MC = false;
    io.BA = false;

    for (;;) {
        ActualizarLampara(ESTADO_ABRIENDO);
        status.cntRunTimer++;
        if (io.LSA) return ESTADO_ABIERTO;
        if (status.cntRunTimer > config.RunTimer) {
            codigoError = 1;
            return ESTADO_ERR;
        }
        if (io.BC || io.PP || io.SE) return ESTADO_STOP;
    }
}

int Func_ESTADO_CERRADO(void) {
    EstadoAnterior = EstadoActual;
    EstadoActual = ESTADO_CERRADO;
    ApagarMotores();
    io.BA = false;

    for (;;) {
        if (io.BA || io.PP) return ESTADO_ABRIENDO;
        if (io.SE) return ESTADO_STOP;
    }
}

int Func_ESTADO_ABIERTO(void) {
    EstadoAnterior = EstadoActual;
    EstadoActual = ESTADO_ABIERTO;
    ApagarMotores();
    status.cntTimerCA = 0;

    for (;;) {
        ActualizarLampara(ESTADO_ABIERTO);
        status.cntTimerCA++;
        if (status.cntTimerCA > config.TimerCA) return ESTADO_CERRANDO;
        if (io.BC || io.PP) return ESTADO_CERRANDO;
        if (io.SE) return ESTADO_STOP;
    }
}

int Func_ESTADO_ERR(void) {
    EstadoAnterior = EstadoActual;
    EstadoActual = ESTADO_ERR;
    ApagarMotores();
    io.Buzzer = 1;

    printf("ERROR: Código %d\n", codigoError);

    for (;;) {
        if (!io.SE) return ESTADO_INICIAL;
    }
}

int Func_ESTADO_STOP(void) {
    EstadoAnterior = EstadoActual;
    EstadoActual = ESTADO_STOP;
    ApagarMotores();
    io.Buzzer = 0;

    for (;;) {
        if (!io.SE) return ESTADO_INICIAL;
    }
}

int main() {
    while (1) {
        switch (EstadoSiguiente) {
            case ESTADO_INICIAL:
                EstadoSiguiente = Func_ESTADO_INICIAL(); break;
            case ESTADO_ABIERTO:
                EstadoSiguiente = Func_ESTADO_ABIERTO(); break;
            case ESTADO_ABRIENDO:
                EstadoSiguiente = Func_ESTADO_ABRIENDO(); break;
            case ESTADO_CERRADO:
                EstadoSiguiente = Func_ESTADO_CERRADO(); break;
            case ESTADO_CERRANDO:
                EstadoSiguiente = Func_ESTADO_CERRANDO(); break;
            case ESTADO_ERR:
                EstadoSiguiente = Func_ESTADO_ERR(); break;
            case ESTADO_STOP:
                EstadoSiguiente = Func_ESTADO_STOP(); break;
        }
    }
    return 0;
}