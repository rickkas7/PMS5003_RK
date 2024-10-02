#include "PMS5003_RK.h"

SYSTEM_MODE(AUTOMATIC);
SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler(LOG_LEVEL_TRACE);

PMS5003_RK pms;

void setup() {
    pms
        .withEnablePin(D27)
        .withResetPin(A5)
        .setup();

}

void loop() {

}