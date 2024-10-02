#ifndef __PMS5003_RK_H
#define __PMS5003_RK_H

#include "Particle.h"

class PMS5003_RK {
public:
    class Data {
    public:
        String toString() const;
        union {
            struct {  
                uint16_t pm_1_0;
                uint16_t pm_2_5;
                uint16_t pm_10_0;
                uint16_t pm_1_0_atmospheric;
                uint16_t pm_2_5_atmospheric;
                uint16_t pm_10_0_atmospheric;
                uint16_t count_0_3;
                uint16_t count_0_5;
                uint16_t count_1_0;
                uint16_t count_2_5;
                uint16_t count_5_0;
                uint16_t count_10_0;
            } d;
            uint16_t array[12];
        } data;
    };


    PMS5003_RK();

    virtual ~PMS5003_RK();

    /**
     * @brief Sets a hardware enable pin, must be called before setup()
     * 
     * @param pin Pin to use, such as D2. Default is PIN_INVALID.
     * @return PMS5003_RK& Reference to this object to chain, fluent-style
     */
    PMS5003_RK &withEnablePin(pin_t pin) { enablePin = pin; return *this; };

    /**
     * @brief Sets a hardware enable pin, must be called before setup()
     * 
     * @param pin Pin to use, such as D3. Default is PIN_INVALID.
     * @return PMS5003_RK& Reference to this object to chain, fluent-style
     */
    PMS5003_RK &withResetPin(pin_t pin) { resetPin = pin; return *this; };

    /**
     * @brief Sets a hardware UART serial port to use, must be called before setup()
     * 
     * @param port such as Serial2. Default is Serial1.
     * @return PMS5003_RK& Reference to this object to chain, fluent-style
     */
    PMS5003_RK &withPort(USARTSerial &port) { serial = port; return *this; };

    
    /**
     * @brief Initialize library and port settings
     * 
     * Be sure to use any withXXX() methods before calling this function.
     */
    void setup();
    
    /**
     * @brief Locks the mutex that protects shared resources
     * 
     * This is compatible with `WITH_LOCK(*this)`.
     * 
     * The mutex is not recursive so do not lock it within a locked section.
     */
    void lock() { os_mutex_lock(mutex); };

    /**
     * @brief Attempts to lock the mutex that protects shared resources
     * 
     * @return true if the mutex was locked or false if it was busy already.
     */
    bool tryLock() { return os_mutex_trylock(mutex); };

    /**
     * @brief Unlocks the mutex that protects shared resources
     */
    void unlock() { os_mutex_unlock(mutex); };



protected:
    /**
     * This class  cannot be copied
     */
    PMS5003_RK(const PMS5003_RK&) = delete;

    /**
     * This class cannot be copied
     */
    PMS5003_RK& operator=(const PMS5003_RK&) = delete;

    /**
     * @brief Worker thread function
     * 
     * This method is called to perform operations in the worker thread.
     * 
     * You generally will not return from this method.
     */
    os_thread_return_t threadFunction(void);

    /**
     * @brief Mutex to protect shared resources
     * 
     * This is initialized in setup() so make sure you call the setup() method from the global application setup.
     */
    os_mutex_t mutex = 0;

    /**
     * @brief Worker thread instance class
     * 
     * This is initialized in setup() so make sure you call the setup() method from the global application setup.
     */
    Thread *thread = 0;

    static const uint8_t START1 = 0x42;
    static const uint8_t START2 = 0x4D;

    static const uint8_t CMD_READ_PASSIVE = 0xe2;
    static const uint8_t CMD_CHANGE_MODE = 0xe1;
    static const uint8_t CMD_SLEEP = 0xe4;

    static const size_t frameBufSize = 30;
    uint8_t frameBuf[frameBufSize];
    size_t frameOffset = 0;
    uint16_t frameLengthRead = 0;

    Data lastData;
    unsigned long lastDataMillis = 0;

    USARTSerial &serial = Serial1;
    int baud = 9600;
    pin_t enablePin = PIN_INVALID;
    pin_t resetPin = PIN_INVALID;

};

#endif // __PMS5003_RK_H