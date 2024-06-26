// ****************************************************************************
//
// Copyright (c) 2023, Septentrio
// Copyright (c) 2013-2022 Niels Lohmann
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.

// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// ****************************************************************************

#include "mqtt.hpp"


void mqtt_on_connect(struct mosquitto *mqttClient, void *userdata, int result) {
    if (result == 0) {
        std::cout << "Connected to broker.\n\nSubscribing to topics ... \n" << std::endl;

        // Access the userdata object
        UserData *user_data = (UserData *)userdata;

        // Only subscribe if not localized or in Lband mode
        if (!user_data->localized || (user_data->corrections_mode == "Lb" ||user_data->corrections_mode == "Dual")){
            // Dynamic Key Topic and QoS = 
            result = mosquitto_subscribe(mqttClient, NULL, user_data->keyTopic.c_str(), user_data->keyQoS);
            if (result != MOSQ_ERR_SUCCESS) {
                std::cerr << "\nError subscribing to " << user_data->keyTopic.c_str() << " topic.\n" << std::endl;

            } else { 
                std::cout << "Subscribed to topic: " << user_data->keyTopic.c_str() << std::endl;
                std::cout << "QoS of the topic: 1\n" << std::endl;
            }
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
            
            // We subscribe only if it is not LBand Only Mode and if localized is disable 
            if ((user_data->corrections_mode == "Ip" || user_data->corrections_mode == "Dual") && !user_data->localized ) { 
                // Corrections Topic and QoS = 0
                result = mosquitto_subscribe(mqttClient, NULL, user_data->corrTopic.c_str(), user_data->corrQoS);
                if (result != MOSQ_ERR_SUCCESS) {
                    std::cerr << "\nError subscribing to " << user_data->corrTopic.c_str() << " topic.\n" << std::endl;

                } else { 
                    std::cout << "Subscribed to topic: " << user_data->corrTopic.c_str() << std::endl;
                    std::cout << "QoS of the topic: 0\n" << std::endl;
                }
            }
            if(user_data->nodeTopic != "" && user_data->localized && (user_data->corrections_mode == "Ip" || user_data->corrections_mode == "Dual") ){
                result = mosquitto_subscribe(mqttClient, NULL, user_data->nodeTopic.c_str(), user_data->nodeQoS);
                if (result != MOSQ_ERR_SUCCESS) {
                    std::cerr << "\nError subscribing to " << user_data->nodeTopic.c_str() << " topic.\n" << std::endl;

                } else { 
                    std::cout << "Subscribed to topic: " << user_data->nodeTopic.c_str() << std::endl;
                    std::cout << "QoS of the topic: 0\n" << std::endl;
                }
            }
        

    } else {
        std::cout << "Connection failed with error code: " << result << "\n" << std::endl;
    }
}

void mqtt_on_message(struct mosquitto *mqttClient, void *userdata, const struct mosquitto_message *message) {
    // Access the userdata object
    UserData *user_data = (UserData *)userdata;


    struct mqttMessgae toPush;
    toPush.topic = std::string(message->topic);
    toPush.payload = std::string((char *) message->payload, message->payloadlen);
    toPush.payloadlen = message->payloadlen;
    
    user_data->message_queue_mutex.lock();
    user_data->message_queue.push(toPush);
    user_data->message_queue_mutex.unlock();    
    user_data->cv_incoming_data->notify_all();

    if(message->topic == user_data->tileTopic){
        mosquitto_unsubscribe(mqttClient , NULL, user_data->tileTopic.c_str()) ;
    }

}