#include "PMS5003_RK.h"

static Logger pmsLog("app.PMS5003");

PMS5003_RK::Data::Data() {
    memset(&data.array, 0, sizeof(data.array));
}


String PMS5003_RK::Data::toString() const {
    return String::format("pm1.0=%d pm2.5=%d pm10.0=%d count 0.3=%d 0.5=%d 1.0=%d 2.5=%d 5.0=%d 10.0=%d",
        (int) data.d.pm_1_0,
        (int) data.d.pm_2_5,
        (int) data.d.pm_10_0,
        // (int) data.d.pm_1_0_atmospheric,
        // (int) data.d.pm_2_5_atmospheric,
        // (int) data.d.pm_10_0_atmospheric,
        (int) data.d.count_0_3,
        (int) data.d.count_0_5,
        (int) data.d.count_1_0,
        (int) data.d.count_2_5,
        (int) data.d.count_5_0,
        (int) data.d.count_10_0
    );
}

void PMS5003_RK::Data::toJSON(JSONWriter &writer) const {
    writer.name("pm1.0").value(data.d.pm_1_0);
    writer.name("pm2.5").value(data.d.pm_2_5);
    writer.name("pm10").value(data.d.pm_10_0);
}


PMS5003_RK::PMS5003_RK() {

}

PMS5003_RK::~PMS5003_RK() {

}

void PMS5003_RK::setup() {
    serial.begin(serialBaud); 

    pmsLog.info("setup called enablePin=%d resetPin=%d", enablePin, resetPin);

    if (enablePin != PIN_INVALID) {
        pinMode(enablePin, OUTPUT);
        digitalWrite(enablePin, HIGH);
    }
    if (resetPin != PIN_INVALID) {
        pinMode(resetPin, OUTPUT);
        digitalWrite(resetPin, HIGH);
    }
    

    os_mutex_create(&mutex);

    thread = new Thread("PMS5003", [this]() { return threadFunction(); }, OS_THREAD_PRIORITY_DEFAULT, 3072);
}

void PMS5003_RK::getLastData(PMS5003_RK::Data &lastData, unsigned long &lastDataMillis) {

    lock();
    lastData = this->lastData;
    lastDataMillis = this->lastDataMillis;
    unlock();
}


void PMS5003_RK::sendCommandWake() {
    sendCommand(CMD_SLEEP, 0x01);
}


void PMS5003_RK::sendCommandMode(bool activeMode) {
    sendCommand(CMD_CHANGE_MODE, activeMode);
}


void PMS5003_RK::sendCommand(uint8_t cmd, uint8_t dataL, uint8_t dataH) {
    uint8_t buf[8];
    buf[0] = START1;
    buf[1] = START2;
    buf[2] = cmd;
    buf[3] = dataH;
    buf[4] = dataL;

    uint16_t sum = 0;
    for(size_t ii = 0; ii < 5; ii++) {
        sum += buf[ii];
    }
    buf[5] = (uint8_t)(sum >> 8);
    buf[6] = (uint8_t) sum;

    for(size_t ii = 0; ii < 7; ii++) {
        serial.write(buf[ii]);
    }
    pmsLog.trace("sendCommand cmd=0x%02x dataL=0x%2x", cmd, dataL);
    // pmsLog.dump(buf, 7); pmsLog.print("\n");
}



os_thread_return_t PMS5003_RK::threadFunction(void) {
    while(true) {
        // Put your code to run in the worker thread here
        if (serial.available()) {
            frameBuf[frameOffset++] = serial.read();
            // pmsLog.trace("serial read offset=%d value=0x%02x", frameOffset - 1, frameBuf[frameOffset - 1]);

            if (frameOffset == 1) {
                if (frameBuf[0] != START1) {                    
                    frameOffset = 0;
                }
                continue;
            }

            if (frameOffset == 2) {
                if (frameBuf[1] != START2) {
                    pmsLog.info("bad START2 got 0x%02x", frameBuf[1]);
                    frameOffset = 0;
                }
                continue;
            }

            if (frameOffset == 4) {
                uint16_t frameLengthRead = (frameBuf[2] << 8) | frameBuf[3];
                if (frameLengthRead != 28) { // 0x1C
                    pmsLog.info("bad frame length got 0x%04x (%d)", (int)frameLengthRead, (int)frameLengthRead);
                    frameOffset = 0;
                }
                // We could use the frame length to process larger frames and ignore the new
                // data, but don't actually do that here.
            }

            if (frameOffset == frameBufSize) {
                // End of frame
                uint16_t checksumCalculated = 0;
                for(size_t ii = 0; ii < (frameBufSize - 2); ii++) {
                    checksumCalculated += frameBuf[ii];
                }
                // 0000020547 [app.PMS5003] INFO: bad checksum calc=0x00c0 exp=0x9700
                // 424d001c000000000000000000000000000f000400010001000000009700

                uint16_t checksumExpected = (frameBuf[30] << 8) | frameBuf[31];

                if (checksumCalculated == checksumExpected) {
                    lock();
                    for(size_t ii = 0; ii < 13; ii++) {
                        lastData.data.array[ii] = (frameBuf[4 + ii * 2] << 8) | frameBuf[5 + ii * 2];
                    }
                    lastDataMillis = millis();
                    unlock();

                    pmsLog.trace("valid %s", lastData.toString().c_str());
                }
                else {
                    pmsLog.info("bad checksum calc=0x%04x exp=0x%04x", (int)checksumCalculated, (int)checksumExpected);
                    pmsLog.dump(frameBuf, frameBufSize); 
                    pmsLog.print("\n");
                }
                frameOffset = 0;
            }
        }
        
        delay(1);
    }
}