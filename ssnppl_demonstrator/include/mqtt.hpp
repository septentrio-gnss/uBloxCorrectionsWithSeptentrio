#ifndef __MQTT__
#define __MQTT__
// General Standard Libraries 
#include <string>
#include <cstring>
#include <iostream>
// JSON sudo apt install nlohmann-json3-dev (DEBIAN 11)
#include <nlohmann/json.hpp>

// MQTT sudo apt-get install libmosquitto-dev (DEBIAN 11)
#include <mosquitto.h> 
#include <queue>
#include <mutex>


struct mqttMessgae {
    std::string topic;
    std::string payload;
    int payloadlen;
};

struct UserData {

    // Program Logic Mode
    std::string corrections_mode;
    std::string region;

    // Dynamic Key MQTT Info
    const std::string keyTopic = "/pp/key/Lb"; //"/pp/key/Lb";
    const int keyQoS = 1;

    // Frequency MQTT Info
    const std::string freqTopic = "/pp/frequencies/Lb";
    const int freqQoS = 1;

    // SPARTN MQTT Info
    const std::string corrTopic = "/pp/Lb/eu";
    const int corrQoS = 0;

    //PPL
    std::queue<struct mqttMessgae> message_queue;
    std::mutex message_queue_mutex;

};

void mqtt_on_connect(struct mosquitto *mqttClient, void *userdata, int result);

void mqtt_on_message(struct mosquitto *mqttClient, void *userdata, const struct mosquitto_message *message);

#endif