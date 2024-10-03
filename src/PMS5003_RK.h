#ifndef __PMS5003_RK_H
#define __PMS5003_RK_H

#include "Particle.h"

/**
 * @brief Class for interfacing with a PMS5003 particulate matter sensor. Also works with PMS7003.
 */
class PMS5003_RK {
public:
    /**
     * @brief Container to hold the result data from the sensor
     */
    class Data {
    public:
        /**
         * @brief Constructor zeros out all of the values
         */
        Data();

        /**
         * @brief Returns a string representation of the data
         * 
         * @return String 
         */
        String toString() const;

        /**
         * @brief Writes keys pm1.0, pm2.5, and pm10 to the JSONWriter
         * 
         * @param writer JSON object to write to
         */
        void toJSON(JSONWriter &writer) const;

        /**
         * @brief Data structure
         */
        union {
            struct {  
                uint16_t pm_1_0; //!< PM 1.0 in ug/m^3
                uint16_t pm_2_5; //!< PM 2.5 in ug/m^3
                uint16_t pm_10_0; //!< PM 10 in ug/m^3
                uint16_t pm_1_0_atmospheric; //!< PM 1.0 in ug/m^3 under atmospheric environment 
                uint16_t pm_2_5_atmospheric; //!< PM 2.5 in ug/m^3 under atmospheric environment 
                uint16_t pm_10_0_atmospheric; //!< PM 10 in ug/m^3 under atmospheric environment 
                uint16_t count_0_3; // Number of particles with diameter greater than 0.3 um in 0.1L of air
                uint16_t count_0_5; // Number of particles with diameter greater than 0.5 um in 0.1L of air
                uint16_t count_1_0; // Number of particles with diameter greater than 1.0 um in 0.1L of air
                uint16_t count_2_5; // Number of particles with diameter greater than 2.5 um in 0.1L of air
                uint16_t count_5_0; // Number of particles with diameter greater than 5.0 um in 0.1L of air
                uint16_t count_10_0; // Number of particles with diameter greater than 10.0 um in 0.1L of air
                uint16_t reserved; // Not used on PMS5003
            } d; //!< Values returned by the sensor
            uint16_t array[13]; //!< Array of uint16_t values so they can be written regardless of byte order
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
     * @brief Get the last data received from the sensor
     * 
     * @param lastData 
     * @param lastDataMillis 
     * 
     * This method is effectively const, however because it needs to obtain a mutex
     * compatible with WITH_LOCK() it cannot be const.
     */
    void getLastData(PMS5003_RK::Data &lastData, unsigned long &lastDataMillis);

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

    /**
     * @brief Send a wake command to the sensor
     */
    void sendCommandWake();

    /**
     * @brief Sets active or passive mode
     * 
     * @param activeMode 
     */
    void sendCommandMode(bool activeMode);

    /**
     * @brief Send a command to the sensor
     * 
     * @param cmd Command code such as CMD_SLEEP
     * @param dataL 
     * @param dataH (always 0 for the PMS5003)
     */
    void sendCommand(uint8_t cmd, uint8_t dataL, uint8_t dataH = 0);

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


    static const uint8_t START1 = 0x42; //!< Start byte for serial command or response
    static const uint8_t START2 = 0x4D; //!< Second start byte

    static const uint8_t CMD_READ_PASSIVE = 0xe2; //!< Command to read sensor data in passive mode
    static const uint8_t CMD_CHANGE_MODE = 0xe1; //!< Command to change mode between active (default) and passive (you need to request data)
    static const uint8_t CMD_SLEEP = 0xe4; //!< Command to enter sleep mode from software

    static const size_t frameBufSize = 32; //!< Size of a complete frame
    uint8_t frameBuf[frameBufSize]; //!< Buffer to hold a frame
    size_t frameOffset = 0; //!< Where in the frame we are currently writing to

    Data lastData; //!< Last data that was received
    unsigned long lastDataMillis = 0; //!< Millis time when last data was received

    USARTSerial &serial = Serial1; //!< Serial port to use (default: Serial1)
    int serialBaud = 9600; //!< Serial baud rate
    pin_t enablePin = PIN_INVALID; //!< Optional pin that is connected to the enable pin on the sensor
    pin_t resetPin = PIN_INVALID; //!< Optional pin that is connected to the reset pin on the sensor
};

#endif // __PMS5003_RK_H