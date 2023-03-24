#include "../queue/queue.hpp"

// JSON sudo apt install nlohmann-json3-dev (DEBIAN 11)
#include <nlohmann/json.hpp>

// MQTT sudo apt-get install libmosquitto-dev (DEBIAN 11)
#include <mosquitto.h> 

#define MAX_CORR_QUEUE_SIZE 10

extern MyQueue corrQueue(MAX_CORR_QUEUE_SIZE); // declare corrQueue as an extern variable

struct UserData {

    // Program Logic Mode
    std::string corrections_mode;
    std::string region;

    // Dynamic Key MQTT Info
    const std::string keyTopic = "/pp/key/Lb"; //"/pp/key/Lb";
    const int keyQoS = 1;
    std::string keyInfo = "";

    // Frequency MQTT Info
    const std::string freqTopic = "/pp/frequencies/Lb";
    const int freqQoS = 1;
    std::string freqInfo = "";

    // SPARTN MQTT Info
    const std::string corrTopic = "/pp/Lb/eu";
    const int corrQoS = 0;

};

std::string waitForTopic(UserData &user_data, const std::string &topic){
    std::string currentContent, str;
    std::string* ptr_topic = nullptr;

    if(topic == user_data.keyTopic) ptr_topic = &user_data.keyInfo;
    else if (topic == user_data.freqTopic) ptr_topic = &user_data.freqInfo;

    // Search for topic information
    int count = 0;
    while(count < 10){
        // Wait
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // Store value
        if(*ptr_topic != "") str = *ptr_topic;

        // Check values
        if(str != "") break;
        else count++;    
    }

    return str;
}

/*---------------------------------------------------------------------
|               MQTT Custom Callbacks                                 |
----------------------------------------------------------------------*/
void on_connect(struct mosquitto *mqttClient, void *userdata, int result) {

    if (result == 0) {
        std::cout << "Connected to broker.\n\nSubscribing to topics ... \n" << std::endl;

        // Access the userdata object
        struct UserData *user_data = (struct UserData *)userdata;

        // Dynamic Key Topic and QoS = 1
        result = mosquitto_subscribe(mqttClient, NULL, user_data->keyTopic.c_str(), user_data->keyQoS);
        if (result != MOSQ_ERR_SUCCESS) {
            std::cerr << "\nError subscribing to " << user_data->keyTopic.c_str() << " topic.\n" << std::endl;

        } else { 
            std::cout << "Correctly Subscribed to topic: " << user_data->keyTopic.c_str() << std::endl;
            std::cout << "QoS of the topic: " <<  user_data->keyQoS << "\n" << std::endl;
        }
        
        // We subscribe to Frequency topic ONLY if it is NOT Ip Only Mode
        if (user_data->corrections_mode != "Ip"){    
            // Frequency Topic and QoS = 1
            result = mosquitto_subscribe(mqttClient, NULL, user_data->freqTopic.c_str(), user_data->freqQoS);
            if (result != MOSQ_ERR_SUCCESS) {
                std::cerr << "\nError subscribing to " << user_data->freqTopic.c_str() << " topic.\n" << std::endl;

            } else { 
                std::cout << "Correctly Subscribed to topic: " << user_data->freqTopic.c_str() << std::endl;
                std::cout << "QoS of the topic: " <<  user_data->freqQoS << "\n" << std::endl;
            }
        }
        
        // We subscribe to Corrections topic ONLY if it is NOT LBand ONLY Mode
        if (user_data->corrections_mode != "Lb") { 
            // Corrections Topic and QoS = 0
            result = mosquitto_subscribe(mqttClient, NULL, user_data->corrTopic.c_str(), user_data->corrQoS);
            if (result != MOSQ_ERR_SUCCESS) {
                std::cerr << "\nError subscribing to " << user_data->corrTopic.c_str() << " topic.\n" << std::endl;

            } else { 
                std::cout << "Correctly Subscribed to topic: " << user_data->corrTopic.c_str() << std::endl;
                std::cout << "QoS of the topic: " <<  user_data->corrQoS << "\n" << std::endl;
            }
        }

    } else { std::cout << "Connection failed with error code: " << result << "\n" << std::endl;}
}

void on_message(struct mosquitto *mqttClient, void *userdata, const struct mosquitto_message *message) {
    // Access the userdata object
    struct UserData *user_data = (struct UserData *)userdata;

    /* Writting the payload of each topics into the struct's variables
    std::cout << "\nNew MQTT Message reveiced." << std::endl;
    std::cout << "  Topic Name: " << message->topic << std::endl;
    std::cout << "  Topic Size: " << message->payloadlen << std::endl; */
    std::cout << std::endl;

    std::string str_message;

    if (message->topic == user_data->freqTopic) {
        // Parse the JSON string
        str_message = std::string((char *) message->payload, message->payloadlen);
        nlohmann::json json = nlohmann::json::parse(str_message);
        // JSON comes in a string format, then convert it to float (std::stof) to not lose decimals when changing the unit to Hz, 
        // translate it to int number (static_cast<int>) bc the receiver only accepts integer number and finally return it as a string (std::to_string).
        std::string freqValue = json["frequencies"][user_data->region]["current"]["value"];
        user_data->freqInfo = std::to_string(static_cast<int>(std::stof(freqValue) * 1000000)); // in Hertz

    } else if (message->topic == user_data->keyTopic){
        // Parse the JSON string
        str_message = std::string((char *) message->payload, message->payloadlen);
        nlohmann::json json = nlohmann::json::parse(str_message);
        user_data->keyInfo = json["dynamickeys"]["current"]["value"]; // It is already an string

    } else if (message->topic == user_data->corrTopic) {
        // Cast message payload to uint8_t* and add the payload to queue
        uint8_t *payload = (uint8_t*) message->payload;
        corrQueue.addData(payload, message->payloadlen);
    }
}

/*
void on_subscribe(struct mosquitto *mqttClient, void *userdata, int mid, int qos_count, const int *granted_qos) {
    std::cout << "Subscribed to topic." << std::endl;

    // Access the userdata object
    struct UserData *user_data = (struct UserData *)userdata;

}
*/