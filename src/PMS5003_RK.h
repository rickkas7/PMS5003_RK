#ifndef __PMS5003_RK_H
#define __PMS5003_RK_H

#include "Particle.h"

class PMS5003_RK {
public:
    


protected:
    USARTSerial &serial = Serial1;
    int baud = 9600;
    pin_t enablePin = PIN_INVALID;
    pin_t resetPin = PIN_INVALID;
};

#endif // __PMS5003_RK_H