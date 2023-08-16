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