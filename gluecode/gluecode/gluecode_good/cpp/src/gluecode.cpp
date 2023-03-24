// *****************************************************************************
//
// © Copyright 2020, Septentrio NV/SA.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//    1. Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//    2. Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//    3. Neither the name of the copyright holder nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// ****************************************************************************

/*
g++ -std=c++11 -o ./build/gluecode gluecode.cpp -I../pp/PPL_64_Bit/inc/-I../include/mqtt/ -I../include/comms/ -L../include/pp/PPL_64_Bit/lib/ ../include/comms/SerialComm.cpp -lmosquitto -lpointperfect -lboost_system -lboost_thread -pthread -lboost_program_options
 
./build/gluecode --mode Ip --main_comm USB --main_config /dev/ttyACM0@115200 --client_id 95b99023-dcc5-4710-8eef-5cfbdd362c36 
./build/gluecode --mode Lb --main_comm USB --main_config /dev/ttyACM0@115200 --client_id 95b99023-dcc5-4710-8eef-5cfbdd362c36 --lband_comm USB --lband_config /dev/ttyACM1@115200

*/

// General Standard Libraries
#include <string>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <vector>
#include <cstring>
#include <csignal>
#include <chrono>
#include <thread>
#include <fstream>

// Circular buffer
#include "../include/buffers/binCircularBuffer.hpp"

// JSON sudo apt install nlohmann-json3-dev (DEBIAN 11)
#include <nlohmann/json.hpp>

// BOOST Libraries
#include <boost/program_options.hpp>

// MQTT
#include <mosquitto.h> // sudo apt-get install libmosquitto-dev (DEBIAN 11)

// SERIAL
#include "../comms/SerialComm.hpp" 

// PPL
#include "../pp/PPL_64_Bit/inc/PPL_PublicInterface.h" // PointPerfect Library

/*---------------------------------------------------------------------
|               Global Variables                                      |
----------------------------------------------------------------------*/
struct UserData {

    // Program Logic Mode
    std::string corrections_mode;
    std::string region;

    // Dynamic Key MQTT Info
    const std::string keyTopic = "/pp/key/Lb";
    std::string keyInfo = "";
    int keyQoS = 1;

    // Frequency MQTT Info
    const std::string freqTopic = "/pp/frequencies/Lb";
    std::string freqInfo = "";
    int freqQoS = 1;

    // SPARTN MQTT Info
    const std::string corrTopic = "/pp/Lb/eu";

    // Constructor that initializes the buffer with a default length
    CircularBuffer corrCircBuff;
    int corrQoS = 0;

};

struct SPARTN_LOG {

    std::ofstream SPARTN_file_Ip;
    std::ofstream SPARTN_file_Lb;

    void closeAll() {
        SPARTN_file_Ip.close();
        SPARTN_file_Lb.close();
    }

};

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
            std::cout << "Subscribed to topic: " << user_data->keyTopic.c_str() << std::endl;
            std::cout << "QoS of the topic: 1\n" << std::endl;
        }
        
        // We subscribe only if it is not Ip Only Mode
        if (user_data->corrections_mode != "Ip"){    
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
        if (user_data->corrections_mode != "Lb") { 
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

void on_message(struct mosquitto *mqttClient, void *userdata, const struct mosquitto_message *message) {
    // Access the userdata object
    struct UserData *user_data = (struct UserData *)userdata;

    // Writting the payload of each topics into the struct's variables
    std::cout << "\nNew MQTT Message reveiced." << std::endl;
    std::cout << "  Topic Name: " << message->topic << std::endl;
    std::cout << "  Topic Size: " << message->payloadlen << std::endl;
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
        // Copy the payload into the corrCircBuff.
        str_message = std::string((char *) message->payload, message->payloadlen);

        // Create a temporary array.
        size_t temp_array_length = str_message.length();
        uint8_t temp_array[temp_array_length];

        // Copy the content of the topic in a temporary array.
        std::copy(str_message.begin(), str_message.end(), temp_array);

        // Add the content of temporary array to circular buffer.
        user_data->corrCircBuff.Add(temp_array, temp_array_length);

    }
}

void on_subscribe(struct mosquitto *mqttClient, void *userdata, int mid, int qos_count, const int *granted_qos) {
    std::cout << "Subscribed to topic." << std::endl;

    // Access the userdata object
    struct UserData *user_data = (struct UserData *)userdata;

}

/**
 * Splits a string into multiple substrings based on a given separator.
 *
 * @param str The string to be split.
 * @param separator The separator character.
 * @return A vector of substrings resulting from the split.
 */
std::vector<std::string> split(const std::string& str, char separator) {
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string item;

    while (std::getline(ss, item, separator)) {
        result.push_back(item);
    }

    return result;
}

bool is_empty(const uint8_t* arr, std::size_t size) {
  uint8_t* zeros = new uint8_t[size]();  // Allocate an array of size 'size' filled with zeros.
  int result = memcmp(arr, zeros, size);
  delete[] zeros;
  return result == 0;
}

char* str2char(const std::string& str) {
    char* charArray = new char[str.length() + 1];
    std::strcpy(charArray, str.c_str());
    return charArray;
}

void print_binary(const uint8_t* data, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        std::bitset<8> bits(data[i]);
        std::cout << bits << " ";
    }
    std::cout << std::endl;
}

void show_options(const boost::program_options::variables_map &vm)
{
    std::cout << "\nCURRENT PROGRAM OPTIONS:" << std::endl;
    std::cout << "---------------------------------------------------" << std::endl;
    std::cout << "GENERAL OPTIONS:" << std::endl;
    std::cout << "  mode:               " << vm["mode"].as<std::string>() << std::endl;

    if(vm["echo"].as<bool>() == true) std::cout << "  echo:                 " << "True" << std::endl;
    else std::cout << "  echo:              " << "False" << std::endl;

    if(vm["reset_default"].as<bool>() == true) std::cout << "  reset_default:       " << "True" << std::endl;
    else std::cout << "  reset_default:     " << "False" << std::endl;

    if(vm["send_cmds"].as<bool>() == true) std::cout << "  send_cmds:           " << "True" << std::endl;
    else std::cout << "  send_cmds:         " << "False" << std::endl;

    if(vm["timer"].as<int>() > 0) std::cout << "  timer:                " << vm["timer"].as<int>() << " Seconds" << std::endl;
    else std::cout << "  timer:             " << "Disabled" << std::endl;
    
    std::cout << "\nLOGING OPTIONS:\n" << std::endl;
    std::cout << "  SPARTN_Logging:     " << vm["SPARTN_Logging"].as<std::string>() << std::endl;
    std::cout << "  SBF_Logging:        " << vm["SBF_Logging"].as<std::string>() << std::endl;
    std::cout << "  SBF_Logging_Config: " << vm["SBF_Logging_Config"].as<std::string>() << std::endl;

    std::cout << "\nMAIN CHANNEL:\n" << std::endl;
    std::cout << "  main_comm:          " << vm["main_comm"].as<std::string>() << std::endl;
    std::cout << "  main_config:        " << vm["main_config"].as<std::string>() << std::endl;

    std::cout << "\nLBAND CHANNEL:\n" << std::endl;
    std::cout << "  lband_comm:         " << vm["lband_comm"].as<std::string>() << std::endl;
    std::cout << "  lband_config:       " << vm["lband_config"].as<std::string>() << std::endl;

    std::cout << "\nMQTT SERVER:\n" << std::endl;
    std::cout << "  client_id:          " << vm["client_id"].as<std::string>() << std::endl;
    std::cout << "  mqtt_server:        " << vm["mqtt_server"].as<std::string>() << std::endl;
    std::cout << "  region:             " << vm["region"].as<std::string>() << std::endl;
    std::cout << "---------------------------------------------------\n\n" << std::endl;
}

void PPL_SendSpartnComplete(uint8_t *array, int arraySize)
{
    // Calculate the number of iterations needed to send all the data in chunks of 100 elements
    int sendElements = arraySize;
    int iterations = sendElements / 100;
    int sent = 0;

    ePPL_ReturnStatus ePPLRet;

    // Loop through the number of iterations
    for (int i = 0; i < iterations; i++) {
        // Send 100 elements of data
        ePPLRet = PPL_SendSpartn(array + (i * 100), 100);
        //ePPLRet = PPL_SendSpartn(mqttUserData.array, MAX_MQTT_SIZE);

        if (ePPLRet != ePPL_Success) {
            std::cout << "FAILED TO SEND CORRECTIONS FROM MQTTT: " << ePPLRet << std::endl;

        } else if (ePPLRet == ePPL_Success) {
            std::cout << "Corrections from MQTT sended to Point Perfect Library." << std::endl;
        }

        // Keep track of the number of elements sent
        sent += 100;

        // Show the number of elements sent
        std::cout << "Sent " << sent << "/" << sendElements << " elements." << std::endl;
    }

    // Send the remaining elements, if any
    if (sendElements % 100 != 0) {
        int remaining = sendElements % 100;
        PPL_SendSpartn(array + (iterations * 100), remaining);
        sent += remaining;

        // Show the number of elements sent
        std::cout << "Sent " << sent << "/" << sendElements << " elements." << std::endl;
    }

    // Zero out the contents of array
    std::fill(array, array + arraySize, 0);
}

int main(int argc, char* argv[])
{

    /******************************************************************************
    
                    CHECK PROGRAM ARGUMENTS AND STORE VALUES

    *******************************************************************************
    * DESCRIPTION: This section of the program is responsible for parsing the     *
    *              program arguments and storing the values.                      *
    *                                                                             *
    *              The Boost.Program_options library is used to define the        *
    *              program parameters and handle the parsing of the arguments.    *
    *                                                                             *
    *              The parsed values are stored in appropriate variables to be    *
    *              used later in the program logic and behavior.                  *
    ******************************************************************************/

    namespace po = boost::program_options;
    po::options_description desc("Allowed options");

    desc.add_options()
        ("help,h", "produce help message")

        // General program Logic
        ("mode", po::value<std::string>()->required(),                                      "mode                       Required | Ip, Lb or Dual.")
        ("echo", po::value<bool>()->default_value(false),                                   "echo                       Optional | true or false.")
        ("reset_default", po::value<bool>()->default_value(true),                           "reset_default              Optional | Set initial config to default if TRUE.")
        ("send_cmds", po::value<bool>()->default_value(true),                               "send_cmds                  Optional | Sends config cmds before main processing loop if TRUE.")
        ("timer", po::value<int>()->default_value(0),                                       "timer                      Optional | Timer to specify how long the program will run, in secs.")

        // Logging Configuration
        ("SPARTN_Logging", po::value<std::string>()->default_value("none"),                 "SPARTN_Logging:            Optional | Introduce Spartn Logfile name.")
        ("SBF_Logging", po::value<std::string>()->default_value("none"),                    "SBF_Logging:               Optional | Introduce SBF Logfile name.")
        ("SBF_Logging_Config", po::value<std::string>()->default_value("none"),             "SBF_Logging_Config:        Optional | If enable_SBF_logging: [Messages@Interval].") //E.g: --SBF_logging_config Support@sec1

        // Main Channel Config
        ("main_comm", po::value<std::string>()->required(),                                 "main_comm:                 Required | USB or Ip")
        ("main_config", po::value<std::string>()->required(),                               "main_config:               Required | If USB: port@baudrate If IP: address@port")
        
        // Lband Channel Config
        ("lband_comm", po::value<std::string>()->default_value("none"),                     "lband_comm:                Optional | USB or Ip")
        ("lband_config", po::value<std::string>()->default_value("none"),                   "lband_config:              Optional | If USB: port@baudrate If IP: address@port")

        // MQTT Config
        ("client_id", po::value<std::string>()->required(),                                 "client_id:                 Required | Your client id")
        ("mqtt_server", po::value<std::string>()->default_value("pp.services.u-blox.com"),  "mqtt_server                Optional | By Default: pp.services.u-blox.com")
        ("region", po::value<std::string>()->default_value("eu"),                           "region                     Optional | By Default: eu");

    po::variables_map vm;

    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        // Show current program options
        show_options(vm);

        std::this_thread::sleep_for(std::chrono::seconds(3));

    } catch (po::error &e) {
        std::cout << "Error: " << e.what() << std::endl << std::endl;
        std::cout << desc << std::endl;
        return 1;
    }

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 0;
    }

    // General program Logic
    std::string mode = vm["mode"].as<std::string>();
    bool echo = vm["echo"].as<bool>();
    bool reset_default = vm["reset_default"].as<bool>();
    bool send_cmds = vm["send_cmds"].as<bool>();
    int timer = vm["timer"].as<int>();

    // Logging Configuration
    std::string SPARTN_Logging = vm["SPARTN_Logging"].as<std::string>();
    std::string SBF_Logging = vm["SBF_Logging"].as<std::string>();
    std::string SBF_Logging_Config = vm["SBF_Logging_Config"].as<std::string>();

    // Main Channel Config
    std::string main_comm = vm["main_comm"].as<std::string>();
    std::string main_config = vm["main_config"].as<std::string>();

    // Lband Channel Config
    std::string lband_comm = vm["lband_comm"].as<std::string>();
    std::string lband_config = vm["lband_config"].as<std::string>();

    // MQTT Config
    std::string client_id = vm["client_id"].as<std::string>();
    std::string mqtt_server = vm["mqtt_server"].as<std::string>();
    std::string region = vm["region"].as<std::string>();

    /******************************************************************************

                    OPEN COMMUNICATION CANNELS

    *******************************************************************************
    * DESCRIPTION: This section of the program is responsible for opening the     *
    *              main communication channel for the device, and the secondary   *
    *              Lband channel if it is enabled using the program input         *
    *              parameters.                                                    *
    *                                                                             *
    *              The main channel is necessary for communication with other     *
    *              devices and is always opened. The Lband channel is optional    *
    *              and is opened only if it is enabled in the program input       *
    *              parameters.                                                    *
    ******************************************************************************/

    // Serial communication instances
    SerialPort main_channel;
    SerialPort lband_channel;

    // Serial communication port names
    std::string receiver_main_port;
    std::string receiver_lband_port;

    // Split Main channel information
    if (main_comm == "USB" && main_comm != "IP"){

        // Get Port and Baudrate
        std::vector<std::string> main_usb_parameters = split(main_config, '@');
        std::string main_usb_port = main_usb_parameters[0];
        unsigned int main_usb_baudrate = std::stoi(main_usb_parameters[1]);

        // Select receiver port name
        receiver_main_port = "USB1";

        // Create serial port object and opten it
        std::cout << "Opening main channel serial port ...\n" << std::endl;
        main_channel.open_serial_port(main_usb_port, main_usb_baudrate);

    } else if (main_comm == "IP" && main_comm != "USB") {
        std::vector<std::string> main_ip_parameters = split(main_config, '@');
        std::string main_ip_addres = main_ip_parameters[0];
        unsigned int main_ip_port = std::stoi(main_ip_parameters[1]);
        // HERE OPEN IP CHANNEL CONFIGURATION

    } else{
        std::cout << "Please insert a correct main channel type: USB or IP. \n" << std::endl;
        return 0;
    }

    // Split Lband channel information
    if (lband_comm != "none"){
        if (lband_comm == "USB"){

            // Get Port and Baudrate
            std::vector<std::string> lband_usb_parameters = split(lband_config, '@');
            std::string lband_usb_port = lband_usb_parameters[0];
            unsigned int lband_usb_baudrate = std::stoi(lband_usb_parameters[1]);

            // Select receiver port name
            receiver_lband_port = "USB2";

            // Create serial port object and opten it
            lband_channel.open_serial_port(lband_usb_port, lband_usb_baudrate);

        } else if (lband_comm == "IP") {
            std::vector<std::string> main_ip_parameters = split(lband_config, '@');
            std::string lband_ip_addres = main_ip_parameters[0];
            unsigned int lband_ip_port = std::stoi(main_ip_parameters[1]);

        } else{
            std::cout << "Please insert a correct lband channel type: USB or IP.";
            return 0;
        }
    }

    /******************************************************************************

                            CREATE AND CONFIGURE MQTT CLIENT

    *******************************************************************************
    * DESCRIPTION: This section of the program is responsible for creating and    *
    *              configuring an MQTT client using the Mosquitto library.        *
    *                                                                             *
    *              The client's configuration and behavior are determined by      *
    *              the program's input parameters, such as the broker address,    *
    *              port, and credentials.                                         *
    *                                                                             *
    *              The client is created using the mosquitto library's            *
    *              mosquitto_new() function, and various options and settings     *
    *              are configured using other library functions.                  *
    *                                                                             *
    *              The MQTT client is a crucial component of the program's        *
    *              communication with the Broker to obtain new information about  *
    *              Dynamic Key, LBand Satellite Frequency and SPARTN Messages,    * 
    *              among others. It must be properly configured and connected     * 
    *              in order for the program to function correctly.                *
    ******************************************************************************/

    // Auth files path information
    const std::string caFile = "../include/mqtt/auth/AmazonRootCA1.pem";
    const std::string certFile = "../include/mqtt/auth/device-" + client_id + "-pp-cert.crt";
    const std::string keyFile = "../include/mqtt/auth/device-" + client_id + "-pp-key.pem";

    // Connection Variables
    const int mqtt_keepalive = 10;
    const int mqtt_port = 8883;
    bool clean_session = true;

    // Initialize Moaquitto library.
    mosquitto_lib_init();
   
    // Create MQTT Client.
    std::cout << "\nCreating MQTT Client.\n" << std::endl;
    struct mosquitto *mosq_client = mosquitto_new(client_id.c_str(), clean_session, NULL);

    // Check for errors.
    if (!mosq_client) {
        std::cerr << "Failed to create new Mosquitto client" << std::endl;
        return 1;
    }

    // Configure MQTT V5 version.
    mosquitto_int_option(mosq_client, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);

    int ret = mosquitto_tls_set(mosq_client, caFile.c_str(), "./" , certFile.c_str(), keyFile.c_str(), NULL);
    if (ret != MOSQ_ERR_SUCCESS) {
        std::cerr << "Failed AUTH to MQTT broker: " << mosquitto_strerror(ret) << std::endl;
        return 1;
    }

    // Create and initialize struct to bind it to userdata object in MQTT Callbacks.
    struct UserData user_data;

    // Set the program logic mode
    user_data.corrections_mode = mode;
    user_data.region = region;

    // Setting the callbacks for the MQTT Client
    mosquitto_message_callback_set(mosq_client, on_message);
    mosquitto_connect_callback_set(mosq_client, on_connect);
    mosquitto_subscribe_callback_set(mosq_client, on_subscribe);

    // set the userdata for the client instance
    mosquitto_user_data_set(mosq_client, &user_data);

    // Establish connection to the broker  
    ret = mosquitto_connect(mosq_client, mqtt_server.c_str(), mqtt_port, mqtt_keepalive);
    if (ret != MOSQ_ERR_SUCCESS) {
        std::cerr << "Failed to connect to MQTT broker: " << mosquitto_strerror(ret) << std::endl;
        return 1;
    }

    /* Starting the client loop                                                               *
    *   Start the main loop of the Mosquitto client. This will cause the client to connect    *
    *   to the broker and listen for messages on the subscribed topics.                       */
    ret = mosquitto_loop_start(mosq_client);
    if (ret != MOSQ_ERR_SUCCESS) {
        std::cerr << "Failed to start main loop of Mosquitto client: " << ret << std::endl;
        return 1;
    }

    // Waint until FIRST Dynamic Key and Frequency Topics Information
    int getFreqAndKey_attemp = 0;

    std::string str_currentDynKey = "";
    std::string str_currentFreq = "";

    std::cout << "Searching for Dynamic Key ";
    if(mode != "Ip") std::cout << "and Frequency topics.\n" << std::endl;
    else std::cout << "topic.\n" << std::endl;

    // Search for Dynamic Key information
    while(getFreqAndKey_attemp < 10){
        // Wait
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // Store value
        if(user_data.keyInfo != "") str_currentDynKey = user_data.keyInfo;

        // Check values
        if(str_currentDynKey != "") {
            //std::cout << str_currentDynKey << std::endl;
            getFreqAndKey_attemp = 0;
            break;

        } else getFreqAndKey_attemp++;
    }

    // Search for Frequency information only if we are using LBand in Lb or Dual Modes
    if (mode != "Ip"){
        getFreqAndKey_attemp = 0;

        while(getFreqAndKey_attemp < 10){
            // Wait
            std::this_thread::sleep_for(std::chrono::seconds(1));

            // Store value
            if(user_data.freqInfo != "") str_currentFreq = user_data.freqInfo;

            // Check values
            if(str_currentFreq != "") {
                std::cout << str_currentFreq << std::endl;
                break;
            } else getFreqAndKey_attemp++;
        }
    }

    /******************************************************************************

                        SEND INITIAL COMMANDS THROUGH MAIN CHANNEL

    *******************************************************************************
    * DESCRIPTION: This section of the program is responsible for preparing a     *
    *              vector of strings, which contais the initial commands to be    *
    *              sent through the main communication channel to the Septentrio  *
    *              Receiver                                                       *
    *                                                                             *
    *              The specific commands sent depend on the program's input       *
    *              arguments, which determine the device configuration and        *
    *              behavior.                                                      *
    *                                                                             *
    *              The commands are written in the SBF (Septentrio Binary Format) *
    *                                                                             *
    *              This section IS NOT a critical component of the program's      *
    *              startup sequence, ONLY IF the user previously has configured   *
    *              the receiver properly, as it initializes and configures the    *
    *              receiver to prepare it for further communication with          *
    *              the program.                                                   *
    ******************************************************************************/

    if (send_cmds == true){

        std::vector<std::string> cmds;

        std::cout << "Starting Receiver configuration with commands ...\n" << std::endl;

        // Default Config Copy
        if (reset_default == true){

            std::cout << "Setting Receiver to Factory Default Config. Sending => eccf, RxDefault, Current" << std::endl;
            main_channel.sync_write("SSSSSSSSSSSSSSSSSSSSSSS\x0D");
            std::this_thread::sleep_for(std::chrono::seconds(2));

            main_channel.sync_write("eccf, RxDefault, Current\x0D");
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }

        // Basic commands
        cmds.push_back("sdio, " + receiver_main_port +", auto, RTCMv3+NMEA\x0D");
        cmds.push_back("sr3o, " + receiver_main_port + ", RTCM1019+RTCM1020+RTCM1042+RTCM1046\x0D");
        cmds.push_back("sno, Stream1, " + receiver_main_port + ", GGA+ZDA, sec1\x0D");

        // Start SBF Logging command, if enabled
        if (SBF_Logging != "none"){

            // Get Messages and Interval
            std::vector<std::string> SBF_Logging_Parameters = split(SBF_Logging_Config, '@');
            std::string SBF_Logging_Messages = SBF_Logging_Parameters[0];
            std::string SBF_Logging_Interval = SBF_Logging_Parameters[1];
            
            // Select SBF Logginf File Name
            cmds.push_back("sfn, DSK1, FileName, " + SBF_Logging + "\x0D");

            // SBF Stream configuration
            cmds.push_back("sso, Stream3, DSK1, " + SBF_Logging_Messages + ", " + SBF_Logging_Interval + "\x0D");
        }

        // Lband TRacking commands
        if (lband_comm != "none" && mode != "Ip"){
            cmds.push_back("slbb, User1, " + str_currentFreq + ", baud2400, , , Enabled\x0D"); //1545260000
            cmds.push_back("slsm, manual, Inmarsat, User1, User2\x0D"); 
            cmds.push_back("slcs, 5555, 6959\x0D");
            cmds.push_back("sdio, " + receiver_lband_port + ", none, LBandBeam1\x0D");
        }

        // Send all configuration commands to receiver
            // Enter command mode
        main_channel.sync_write("SSSSSSSSSSSSSSSSSSSSSSS\x0D");
        std::this_thread::sleep_for(std::chrono::seconds(2));

        //Send commands
        size_t cmds_size = cmds.size();
        std::cout << "\nTotal configuration commands to send: " + std::to_string(cmds_size) + "\n" << std::endl;
        for (int i = 0; i < cmds_size; i++) {
            std::cout << "Sending configuration commands: " + std::to_string(i + 1) + "/" + std::to_string(cmds_size) << " => " << cmds[i] << std::endl;
            main_channel.sync_write(cmds[i]);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    /******************************************************************************

                                POINT PERFECT LIBRARY

    *******************************************************************************
    * DESCRIPTION: This section of the program is responsible for initializing    *
    *              the Point Perfect Library, a uBlox's library used for          *
    *              performing SPARTN decoding and RTCM Corrections calculations   *
    *              based on SPARTN Messages and Satellite Ephemeris and           *
    *              Receiver's NMEA (GGA + ZDA) Messages provided by the receiver. *
    *                                                                             *
    *              The initialization process is dependent on the program's       *
    *              logic mode, which is determined by an input parameter.         *
    *                                                                             *
    *              Additionally, the library is authenticated using a dynamic     *
    *              key that is obtained from a MQTT topic. This authentication    *
    *              process is necessary for operate correctly the library.        *
    *                                                                             *
    *              The Point Perfect Library is a crucial component of the        *
    *              program's core functionality, and its correct initialization   *
    *              and configuration is critical for the program's overall        *
    *              performance.                                                   *
    ******************************************************************************/

    std::cout << "\nInitializing Point Perfect Library ... ";

    ePPL_ReturnStatus ePPLRet;

    // Initialize PointPerfect Library
    if (mode == "Ip") {         ePPLRet = PPL_Initialize(PPL_CFG_ENABLE_IP_CHANNEL);} 
    else if (mode == "Lb") {    ePPLRet = PPL_Initialize(PPL_CFG_ENABLE_AUX_CHANNEL);} 
    else if (mode == "Dual") {  ePPLRet = PPL_Initialize(PPL_CFG_DEFAULT_CFG);}    

    if (ePPLRet!= ePPL_Success) 
    {
        std::cerr << "FAILED. \n" << std::endl;
        return 1;

    } else {
        std::cout << "SUCCESS. \n" << std::endl;
    }

    // Dynamic Key
    char* currentDynKey = str2char(str_currentDynKey);
    uint8_t dynKeyLen = strlen(currentDynKey); 

    // Send Dynamic Key
    std::cout << "Authentication with Dynamic Key ... ";
    ePPLRet = PPL_SendDynamicKey(currentDynKey, dynKeyLen);
    if (ePPLRet!= ePPL_Success) 
    {
        std::cerr << "FAILED. \n" << std::endl;
        std::cout << "PPL Authentication error: " << ePPLRet << std::endl; //Invlid lenght or format (!)
        std::cout << "  - Used Key:   " << currentDynKey << std::endl;
        std::cout << "  - Key lenght: " << dynKeyLen << std::endl;

        return 1;

    } else {
        std::cout << "SUCCESS. \n" << std::endl;
        std::cout << "  - Used Key:   " << currentDynKey << std::endl;
        std::cout << "  - Key lenght: " << dynKeyLen << std::endl;
        std::cout << std::endl;

    }

    /******************************************************************************

                                    SPARTN LOGGING

    *******************************************************************************
    * DESCRIPTION: This section of the program is responsible for setting up      *
    *              logging capabilities for SPARTN messages.                      *
    *                                                                             *
    *              The logging is performed using std::ofstream objects and can   *
    *              capture messages coming from both the LBand beam and MQTT      *
    *              topic.                                                         *
    *                                                                             *
    *              This logging is critical for monitoring and troubleshooting    *
    *              the program's behavior, and can provide valuable insight into  *
    *              any issues that may arise during program execution.            *
    ******************************************************************************/

    // Declare here SPARTN_LOG struct instance to share the scope with the main while proccessing loop.
    SPARTN_LOG spartn_Log_Stream;

    // Set SPARTN Loggin, if enabled
    if(SPARTN_Logging != "none"){

        // Depending on the program logic mode, open file(s)
        if(mode == "Ip") spartn_Log_Stream.SPARTN_file_Ip.open(SPARTN_Logging + "_Ip.bin", std::ios::binary);
        else if(mode == "Lb") spartn_Log_Stream.SPARTN_file_Lb.open(SPARTN_Logging + "_Lb.bin", std::ios::binary);
        else if(mode == "Dual") {
            spartn_Log_Stream.SPARTN_file_Ip.open(SPARTN_Logging + "_Ip.bin", std::ios::binary);
            spartn_Log_Stream.SPARTN_file_Lb.open(SPARTN_Logging + "_Lb.bin", std::ios::binary);
        }
    }

    /******************************************************************************

                                TIMER DECLARATIONS

    *******************************************************************************
    * DESCRIPTION: This section of the program is responsible for declaring and   *
    *              defining variables that are needed to measure time intervals   *
    *              during program execution. The timers are used for various      *
    *              purposes, such as automating program execution, testing the    *
    *              program and receiver performance, and measuring specific       *
    *              time intervals during program operation.                       *
    *                                                                             *
    *              Accurate timing is critical for many types of programs, and    *
    *              can be used to automate program usage in certain ways,         *
    *              or to automate testing scripts for the program and receiver    *
    *              performance.                                                   *
    *                                                                             *
    *              Additionally, timing can help identify bottlenecks and track   *
    *              down bugs and errors in program execution.                     *
    ******************************************************************************/

    // Declare here duration and start variables to share the scope with the main while proccessing loop.
    std::chrono::seconds duration;
    std::chrono::time_point<std::chrono::high_resolution_clock> start;

    // Start timer and assing value to duration and start, if enabled
    if (timer > 0){
        // Set the duration of the loop
        duration = std::chrono::seconds(timer);

        // Get the current time
        start = std::chrono::high_resolution_clock::now();
    }

    /******************************************************************************

                            MAIN WHILE PROCCESSING LOOP

    *******************************************************************************
    * DESCRIPTION:  In this section of the program, the main processing loop      *
    *               is defined, and it runs until the program is stopped by an    *
    *               error, user intervention or a timer. Before the while loop is * 
    *               executed, some buffers are declared to store data.            *
    *                                                                             *
    *               The main processing loop is critical for the proper           *
    *               functioning of the program, as it is responsible for handling *
    *               incoming data and making decisions based on user's            *
    *               configuration, properly handling and processing incoming data *
    *               is crucial for the accuracy and performance of the program.   *
    *                                                                             *
    * STEPS:    1.- The program checks if a time duration specified in the        *
    *               program arguments has been reached, and if so, the program    *
    *               stops running.                                                *
    *                                                                             *
    *           2.- The program checks for a new dynamic key in a MQTT topic,     *
    *               and if one is found, it sends it to the Point Perfect         *
    *               Library for new authentication.                               *
    *                                                                             *
    *           3.- If the program is in LBand or dual mode, it checks if there   *
    *               is new LBand frequency information coming in a MQTT topic     *
    *               and if one is found, it sends it to the receiver via SBF      *
    *               messge for the new LBand frequency configuration.             *
    *                                                                             *
    *           4.- The program reads Ephemeris and NMEA messages that are        *
    *               coming from the receiver, and then sends them to the          *
    *               Point Perfect Library.                                        *
    *                                                                             *
    *           5.- The program reads the new SPARTN datafrom the selected        *
    *               source. This data can come from MQTT Broker or LBand,         *
    *               and depending on the program logic, which is specified        *
    *               by the program input argument.                                *
    *                                                                             *
    *           6.- Finally, the program looks for new RTCM generated messages    *
    *               that are coming from the Point Perfect Library.               *
    *               The library needs Ephemeris, NMEA messages, and SPARTN        *
    *               messages to compute these RTCM messages. Once new RTCM        *
    *               messages are obtained, the program sends them to the          *
    *               receiver using the main communication channel.                *
    *                                                                             *
    ******************************************************************************/

    // Determine the receiver interaction buffer sizes before the main while processing
    uint8_t gnssData[MAX_RCVR_DATA];
    uint8_t lbandData[MAX_RCVR_DATA];

    // Variables to get RTCM from PointPerfect Library
    uint8_t rtcmBuffer[PPL_MAX_RTCM_BUFFER];
    uint32_t rtcmSize = 0;

    // Determine the buffer to store the data coming from corrCircBuff (MQTT Corrections)
        // MQTT Topics -> corrCircBuff -> mqttData -> PointPerfect Library
    const std::size_t mqttData_size = 500;
    uint8_t mqttData[mqttData_size];

    while(true){
        // Timer Check
        if (timer > 0){
            // Check if the duration has been reached
            if (std::chrono::high_resolution_clock::now() - start >= duration) {
                std::string str_duration_secs = std::to_string(std::chrono::duration_cast<std::chrono::seconds>(duration).count());
                std::cout << "\n\nDESIRED DURATION HAS BEEN REACHED.\n" << std::endl;
                std::cout << "Total proccessing time: " + str_duration_secs + " Seconds.\n" << std::endl;

                /* Stop the MQTT client
                ret = mosquitto_loop_stop(mosq_client, false);
                if (ret == MOSQ_ERR_SUCCESS) std::cout << "\nMQTT Client Loop Stopped" << std::endl;
                else std::cout << "\nFailed to stop MQTT Client loop: " << ret << std::endl;

                // Disconnect from the broker and clean up
                ret = mosquitto_disconnect(mosq_client);

                if (ret == MOSQ_ERR_SUCCESS)std::cout << "\nMQTT Client Disconnected" << std::endl;
                else std::cout << "\nFailed to disconnect from the broker and clean up: " << ret << std::endl;

                mosquitto_destroy(mosq_client);
                mosquitto_lib_cleanup(); */

                // Finish SBF Logging, if enabled
                if (SBF_Logging != "none" && send_cmds == true){
                    std::cout << "Stoping SBF Logging ..." << std::endl;
                    
                    main_channel.sync_write("SSSSSSSSSSSSSSSSSSSSSSS\x0D");
                    std::this_thread::sleep_for(std::chrono::seconds(2));

                    main_channel.sync_write("sso, Stream3, none, none, off\x0D");
                    std::this_thread::sleep_for(std::chrono::seconds(1));
    
                }   

                // Finish SPARTN Logging, if enabled
                if (SPARTN_Logging != "none"){
                    std::cout << "Stoping SPARTN Logging ..." << std::endl;

                    // Close the file streams
                    //spartn_Log_Stream.closeAll();
                }           
                
                break;
            }
        }

        // If there is a new Dynamic Key, change it.
        if(user_data.keyInfo != str_currentDynKey) {
            
            str_currentDynKey = user_data.keyInfo;
            currentDynKey = str2char(str_currentDynKey);
            dynKeyLen = sizeof(currentDynKey); 

            ePPLRet = PPL_SendDynamicKey(currentDynKey, dynKeyLen);
            if (ePPLRet!= ePPL_Success) 
            {
                std::cerr << "PPL Authentication FAILED." << std::endl;
                std::cout << "PPL Authentication ERROR Code: " << ePPLRet << std::endl;
                return 1;

            } else {
                std::cout << "PPL Authentication SUCCESS. \n" << std::endl;
            }

            std::cout << "  - Used Key: " << currentDynKey << std::endl;
            std::cout << "  - Key lenght: " << dynKeyLen << std::endl;
        }

        // If there is a new Lband Frequency, change it in the configuration.
        // Also one condition is if we are using LBand ONly or Dual mode
        if(mode != "Ip" && user_data.freqInfo != str_currentFreq) { 
            // Send SSSSSS... command to put receiver's input in Command INput Mode.
            main_channel.sync_write("SSSSSSSSSSSSSSSSSSSSSSS\x0D");
            std::this_thread::sleep_for(std::chrono::seconds(2));

            // Send command with new frequency.
            str_currentFreq = user_data.freqInfo;
            main_channel.sync_write("slbb, User1, " + str_currentFreq + ", baud2400, , , Enabled\x0D");
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // Reading Ephemeris and NMEA Reading from Receiver.
        if (main_comm == "USB"){
            // Synchrnous Serial Port Read. Stores data in a private buffer of the SerialComm Class.
            main_channel.sync_read();

        } else if (main_comm == "IP") {
            std::cout << std::endl;
            // here read from IP channel
        }

        // Copy the conten of private buffer to gnssData
        std::memcpy(gnssData, main_channel.getSyncBuffer(), MAX_RCVR_DATA); // send directly (!)

        // Clean Sync Read Private Buffer
        main_channel.clearSyncBuffer();

        // Check if gnssData is empty
        if (is_empty(gnssData, MAX_RCVR_DATA)){
            std::cout << "No data read from Receiver.\n" << std::endl;

        } else {
            // Send Ephemeris + NMEA data coming from Receiver to PPL.
            ePPL_ReturnStatus ePPLRet = PPL_SendRcvrData(gnssData, MAX_RCVR_DATA);
            if (ePPLRet != ePPL_Success) {
                std::cout << "FAILED TO SEND RCVR DATA"<< std::endl;

            } else {
                std::cout << "\nEphemeris Received.\n" << std::endl;
            }
        }

        // Check if we are using LBand
        if (mode == "Lb" || mode == "Dual"){
            // Reading Raw LBand data from lband_channel.
            if (lband_comm == "USB"){
                // Synchrnous Serial Port Read. Stores data in a private buffer of the SerialComm Class.
                lband_channel.sync_read();

            } else if (lband_comm == "IP"){
                std::cout << std::endl;
                // here read from IP channel
            }

            // Copy the content of private buffer to lbandData
            std::memcpy(lbandData, lband_channel.getSyncBuffer(), MAX_RCVR_DATA);

            if(SPARTN_Logging != "none") spartn_Log_Stream.SPARTN_file_Lb.write((char*)&lbandData, MAX_RCVR_DATA).flush();
            //if(SPARTN_Logging != "none") spartn_Log_Stream.SPARTN_file_Lb.write((char*)&lbandData, MAX_RCVR_DATA);

            // Clean Sync Read Private Buffer
            main_channel.clearSyncBuffer();

            // Check if lbandData is empty
            if (is_empty(lbandData, MAX_RCVR_DATA)){
                std::cout << "No data read from LBand.\n" << std::endl;

            } else {
                // Send LBand data coming from Receiver to PPL.
                ePPL_ReturnStatus ePPLRet = PPL_SendAuxSpartn(lbandData, MAX_RCVR_DATA);
                if (ePPLRet != ePPL_Success) {
                    std::cout << "FAILED TO SEND LBAND DATA: " << ePPLRet << std::endl;

                } else {
                    std::cout << "LBand Received.\n" << std::endl;
                }
            }

        } 

        if (mode == "Ip" || mode == "Dual"){

            // Using Ip Corrections (From MQTT)
            user_data.corrCircBuff.Remove(mqttData, mqttData_size);

            if(SPARTN_Logging != "none") spartn_Log_Stream.SPARTN_file_Lb.write((char*)&mqttData, mqttData_size).flush();
            PPL_SendSpartnComplete(mqttData, mqttData_size);
        
        }

        // Once we sent to PPL al the data it needs, whatever is the mode or the selected channels:
        // Get new RTCM From Point Perfect Library
        ePPL_ReturnStatus ePPLRet = PPL_GetRTCMOutput(rtcmBuffer, PPL_MAX_RTCM_BUFFER, &rtcmSize);
        if (ePPLRet != ePPL_Success) {
            std::cout << "RTCM FAILED" << std::endl;

        } else if (ePPLRet == ePPL_Success && rtcmSize > 0) {
            std::cout << "RTCM SUCCESS. \nRTCM Size: " << rtcmSize << std::endl;

            // Send data to Receiver
            main_channel.sync_write(rtcmBuffer, rtcmSize);

            // Clean RTCM Buffers
            std::memset(rtcmBuffer, 0, PPL_MAX_RTCM_BUFFER);
            rtcmSize = 0;

        }       

    } // End of while(true){

    return 0;
}