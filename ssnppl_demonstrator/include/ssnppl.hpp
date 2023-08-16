#ifndef __GLUECODE__
#define __GLUECODE__

#include "program_option.hpp"
#include "mqtt.hpp"
#include "SerialComm.hpp"
#include <thread>
#include <queue>
#include "PPL_PublicInterface.h" // PointPerfect Library
#include <vector>
#include <atomic>

enum ssnppl_error
{
    SUCCESS,
    FAIL,
    MQTT_ERROR,
    PPL_FAILED,
};

class Ssnppl_demonstrator
{
private:
    // State struct
    ProgramOptions options;
    char *currentDynKey;

    std::string freqInfo = "";
    std::string keyInfo = "";

    std::atomic<bool> update_receiver{false};

    ssnppl_error init_option(int argc, char *argv[]);
    void init_receiver();
    ssnppl_error init_ppl();

    // Serial Port
    SerialPort main_channel;
    std::mutex main_channel_mutex;
    SerialPort lband_channel;
    std::mutex lband_channel_mutex;

    // Ephemeris GGA thread
    void read_ephemeris_gga_data();
    std::thread read_ephemeris_gga_data_thread;
    std::queue<std::vector<uint8_t>> ephemeris_gga_queue;
    std::mutex ephemeris_gga_mutex;

    // LBand thread
    void read_lband_data();
    std::thread read_lband_data_thread;
    std::queue<std::vector<uint8_t>> lband_queue;
    std::mutex lband_queue_mutex;

    // PPL thread
    void handle_data();

    // Send RTCM thread
    void write_rtcm();
    std::thread write_rtcm_thread;
    std::queue<std::vector<uint8_t>> rtcm_queue;
    std::mutex rtcm_queue_mutex;

    ssnppl_error init_main_comm();
    ssnppl_error init_lband_comm();

    // MQTT
    struct mosquitto *mosq_client;
    struct UserData UserData;
    ssnppl_error init_mqtt();

    // SPARTN LOG
    std::ofstream SPARTN_file_Ip;
    std::ofstream SPARTN_file_Lb;

public:
    // Default Ctor and Dtor
    Ssnppl_demonstrator() = default;
    virtual ~Ssnppl_demonstrator();

    // Init func
    ssnppl_error init(int argc, char *argv[]);
    ssnppl_error dispatch_forever();
};

#endif