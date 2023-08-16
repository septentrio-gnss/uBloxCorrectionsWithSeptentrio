
#include "mqtt.hpp"


void mqtt_on_connect(struct mosquitto *mqttClient, void *userdata, int result) {

    if (result == 0) {
        std::cout << "Connected to broker.\n\nSubscribing to topics ... \n" << std::endl;

        // Access the userdata object
        struct UserData *user_data = (struct UserData *)userdata;

        // Dynamic Key Topic and QoS = 1
        result = mosquitto_subscribe(mqttClient, NULL, user_data->keyTopic.c_str(), user_data->keyQoS);
        if (result != MOSQ_ERR_SUCCESS) {
            std::cerr << "\nError subscribing to " << user_data->keyTopic.c_str() << " topic.\n" << std::endl;

        } else { 
            std::cout << "Subscribed to topic: " << user_data->keyTopic.c_str() << std::endl;
            std::cout << "QoS of the topic: 1\n" << std::endl;
        }
        
        // We subscribe only if it is not Ip Only Mode
        if (user_data->corrections_mode == "Lb" || user_data->corrections_mode == "Dual" ){    
            // Frequency Topic and QoS = 1
            result = mosquitto_subscribe(mqttClient, NULL, user_data->freqTopic.c_str(), user_data->freqQoS);
            if (result != MOSQ_ERR_SUCCESS) {
                std::cerr << "\nError subscribing to " << user_data->freqTopic.c_str() << " topic.\n" << std::endl;

            } else { 
                std::cout << "Subscribed to topic: " << user_data->freqTopic.c_str() << std::endl;
                std::cout << "QoS of the topic: 1\n" << std::endl;
            }
        }
        
        // We subscribe only if it is not LBand Only Mode
        if (user_data->corrections_mode == "Ip" || user_data->corrections_mode == "Dual" ) { 
            // Corrections Topic and QoS = 0
            result = mosquitto_subscribe(mqttClient, NULL, user_data->corrTopic.c_str(), user_data->corrQoS);
            if (result != MOSQ_ERR_SUCCESS) {
                std::cerr << "\nError subscribing to " << user_data->corrTopic.c_str() << " topic.\n" << std::endl;

            } else { 
                std::cout << "Subscribed to topic: " << user_data->corrTopic.c_str() << std::endl;
                std::cout << "QoS of the topic: 0\n" << std::endl;
            }
        }

    } else {
        std::cout << "Connection failed with error code: " << result << "\n" << std::endl;
    }
}

void mqtt_on_message(struct mosquitto *mqttClient, void *userdata, const struct mosquitto_message *message) {
    // Access the userdata object
    struct UserData *user_data = (struct UserData *)userdata;


    struct mqttMessgae toPush;
    toPush.topic = std::string(message->topic);
    toPush.payload = std::string((char *) message->payload, message->payloadlen);
    toPush.payloadlen = message->payloadlen;

    user_data->message_queue_mutex.lock();
    user_data->message_queue.push(toPush);
    user_data->message_queue_mutex.unlock();    


    }

