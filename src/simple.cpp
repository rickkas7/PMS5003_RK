#include "PMS5003_RK.h"

SYSTEM_MODE(AUTOMATIC);
SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler(LOG_LEVEL_INFO);

PMS5003_RK pms;

unsigned long lastLogMillis = 0;
const std::chrono::milliseconds logPeriod = 5s;

unsigned long lastPublishMillis = 0;
const std::chrono::milliseconds publishPeriod = 5min;
unsigned long lastPublishLastDataMillis = 0;

void setup() {
    waitFor(Serial.isConnected, 10000); delay(2000);

    pms.setup();
}

void loop() {
    if (millis() - lastLogMillis >= logPeriod.count()) {
        lastLogMillis = millis();

        PMS5003_RK::Data lastData;
        unsigned long lastDataMillis;

        pms.getLastData(lastData, lastDataMillis);
        Log.info("pms %s %d", lastData.toString().c_str(), lastDataMillis);

    }

    if (Particle.connected() && (lastPublishMillis == 0 || (millis() - lastPublishMillis >= publishPeriod.count()))) {
        lastPublishMillis = millis();

        PMS5003_RK::Data lastData;
        unsigned long lastDataMillis;

        pms.getLastData(lastData, lastDataMillis);
        if (lastDataMillis != lastPublishLastDataMillis) {
            lastPublishLastDataMillis = lastDataMillis;
            
            char buf[512];
            memset(buf, 0, sizeof(buf));
            JSONBufferWriter writer(buf, sizeof(buf) - 1);

            writer.beginObject();   
            lastData.toJSON(writer);
            writer.endObject();

            Particle.publish("pmsSensor", buf);
            Log.info("published %s", buf);
        }

    }

}
