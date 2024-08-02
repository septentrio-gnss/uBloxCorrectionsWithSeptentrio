// ****************************************************************************
//
// Copyright (c) 2023, Septentrio
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

#include "ssnppl.hpp"
#include "utils.hpp"
#include <nlohmann/json.hpp>
#include <mosquitto.h>
#include <cmath>
#include <PPL_PublicInterface.h>
#include "mqtt.hpp"

ssnppl_error Ssnppl_demonstrator::init(int argc, char *argv[])
{
    if (init_option(argc, argv) != ssnppl_error::SUCCESS)
    {
        return ssnppl_error::FAIL;
    }

    if (init_main_comm() != ssnppl_error::SUCCESS)
    {
        return ssnppl_error::FAIL;
    }

    if (init_lband_comm() != ssnppl_error::SUCCESS)
    {
        return ssnppl_error::FAIL;
    }
    if (init_ppl() != ssnppl_error::SUCCESS)
    {
        return ssnppl_error::FAIL;
    }
   init_SPARTN_LOG();
    if (init_mqtt() != ssnppl_error::SUCCESS)
    {
        return ssnppl_error::FAIL;
    }
   init_receiver();

   

    return ssnppl_error::SUCCESS;
}

ssnppl_error Ssnppl_demonstrator::init_option(int argc, char *argv[])
{
    // Parse program option
    try
    {
        options = ParseProgramOptions(argc, argv);
        // Show current program options
        showOptions(options);

        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
    catch (po::error &e)
    {
        std::cout << "Error: " << e.what() << std::endl;
        std::cout << "Use --help" << std::endl;

        return ssnppl_error::FAIL;
    }
    return ssnppl_error::SUCCESS;
}

ssnppl_error Ssnppl_demonstrator::init_main_comm()
{

    if (options.main_comm == "USB")
    {
        std::vector<std::string> main_usb_parameters = split(options.main_config, '@');
        std::string main_usb_port = main_usb_parameters[0];
        unsigned int main_usb_baudrate = std::stoi(main_usb_parameters[1]);

        // Create serial port object and opten it
        std::cout << "Opening main channel serial port ..." << std::endl;
        main_channel.open_serial_port(main_usb_port, main_usb_baudrate);
        options.receiver_main_port = "USB1";
    }
    else if (options.main_comm != "IP")
    {
        // error
        std::cout << "Please insert a correct main channel type: USB or IP." << std::endl;
        return ssnppl_error::FAIL;
    }

    return ssnppl_error::SUCCESS;
}

ssnppl_error Ssnppl_demonstrator::init_lband_comm()
{

    if (options.lband_comm == "USB")
    {
        std::vector<std::string> lband_usb_parameters = split(options.lband_config, '@');
        std::string lband_usb_port = lband_usb_parameters[0];
        unsigned int lband_usb_baudrate = std::stoi(lband_usb_parameters[1]);

        // Create serial port object and opten it
        std::cout << "Opening lband channel serial port ...\n"
                  << std::endl;
        lband_channel.open_serial_port(lband_usb_port, lband_usb_baudrate);
    }
    else if (options.lband_comm != "IP" && options.lband_comm != "none")
    {
        // error
        std::cout << "Please insert a correct lband channel type: USB or IP." << std::endl;
        return ssnppl_error::FAIL;
    }

    return ssnppl_error::SUCCESS;
}

ssnppl_error Ssnppl_demonstrator::init_mqtt()
{
    // Auth files path information
    const std::string caFile = options.mqtt_auth_folder + "/AmazonRootCA1.pem";
    const std::string certFile = options.mqtt_auth_folder + "/device-" + options.client_id + "-pp-cert.crt";
    const std::string keyFile = options.mqtt_auth_folder + "/device-" + options.client_id + "-pp-key.pem";

    // Connection Variables
    const int mqtt_keepalive = 10;
    const int mqtt_port = 8883;
    bool clean_session = true;

    // Initialize Moaquitto library.
    mosquitto_lib_init();

    // Create MQTT Client.
    std::cout << "\nCreating MQTT Client.\n"
              << std::endl;
    mosq_client = mosquitto_new(options.client_id.c_str(), clean_session, NULL);

    // Check for errors.
    if (!mosq_client)
    {
        std::cerr << "Failed to create new Mosquitto client" << std::endl;
        return ssnppl_error::MQTT_ERROR;
    }

    // Configure MQTT V5 version.
    mosquitto_int_option(mosq_client, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);

    int ret = mosquitto_tls_set(mosq_client, caFile.c_str(), "./", certFile.c_str(), keyFile.c_str(), NULL);
    if (ret != MOSQ_ERR_SUCCESS)
    {
        std::cerr << "Failed AUTH to MQTT broker: " << mosquitto_strerror(ret) << std::endl;
        return ssnppl_error::MQTT_ERROR;
    }

    // Set the program logic mode
    userData.corrections_mode = options.mode;
    userData.region = options.region;
    userData.cv_incoming_data = &cv_incoming_data;
    // Set Localized distribution 
    userData.localized = options.localized ;

    // Setting the callbacks for the MQTT Client
    mosquitto_message_callback_set(mosq_client, mqtt_on_message);
    mosquitto_connect_callback_set(mosq_client, mqtt_on_connect);

    // set the userData for the client instance
    mosquitto_user_data_set(mosq_client, &userData);

    // Establish connection to the broker
    ret = mosquitto_connect(mosq_client, options.mqtt_server.c_str(), mqtt_port, mqtt_keepalive);
    if (ret != MOSQ_ERR_SUCCESS)
    {
        std::cerr << "Failed to connect to MQTT broker: " << mosquitto_strerror(ret) << std::endl;
        return ssnppl_error::MQTT_ERROR;
    }

    /* Starting the client loop                                                               *
     *   Start the main loop of the Mosquitto client. This will cause the client to connect    *
     *   to the broker and listen for messages on the subscribed topics.                       */
    ret = mosquitto_loop_start(mosq_client);
    if (ret != MOSQ_ERR_SUCCESS)
    {
        std::cerr << "Failed to start main loop of Mosquitto client: " << ret << std::endl;
        return ssnppl_error::MQTT_ERROR;
    }
    userData.mqttServer = options.mqtt_server;
    return ssnppl_error::SUCCESS;
}

ssnppl_error Ssnppl_demonstrator::switch_mqtt_server(std::string new_mqtt_server){

    std::cout<<"\nStop main loop of Mosquitto client" <<std::endl  ;
     int ret = mosquitto_loop_stop(mosq_client,true);
    if (ret != MOSQ_ERR_SUCCESS)
    {
        std::cerr << "Failed to stop main loop of Mosquitto client: " << ret << std::endl;
        return ssnppl_error::MQTT_ERROR;
    }
    std::cout<< " \nDisconnected from old MQTT broker : " << userData.mqttServer <<std::endl;
    ret = mosquitto_disconnect(mosq_client);
    if (ret != MOSQ_ERR_SUCCESS)
    {
        std::cerr << "Failed to disconnect to MQTT broker: " << mosquitto_strerror(ret) << std::endl;
        return ssnppl_error::MQTT_ERROR;
    }
    sleep(2);
    const int mqtt_keepalive = 10;
    const int mqtt_port = 8883;

    std::cout << "\nConnect to new MQTT broker : " << new_mqtt_server  << std::endl ;
     ret = mosquitto_connect(mosq_client, new_mqtt_server.c_str(), mqtt_port, mqtt_keepalive);
    if (ret != MOSQ_ERR_SUCCESS)
    {
        std::cerr << "Failed to connect to MQTT broker: " << mosquitto_strerror(ret) << std::endl;
        return ssnppl_error::MQTT_ERROR;
    }

    /* Starting the client loop                                                               *
     *   Start the main loop of the Mosquitto client. This will cause the client to connect    *
     *   to the broker and listen for messages on the subscribed topics.                       */
    ret = mosquitto_loop_start(mosq_client);
    if (ret != MOSQ_ERR_SUCCESS)
    {
        std::cerr << "Failed to start main loop of Mosquitto client: " << ret << std::endl;
        return ssnppl_error::MQTT_ERROR;
    }
    userData.mqttServer = new_mqtt_server;

    return ssnppl_error::SUCCESS;
}

void Ssnppl_demonstrator::handle_data()
{
    std::unique_lock<std::mutex> mutex{lk_incoming_data};
    // Wait for signal of new data (MQTT or LBAND or GGA/EPH)
    cv_incoming_data.wait(lk_incoming_data);
  
    // Handle MQTT
    {
        std::lock_guard<std::mutex> lock(userData.message_queue_mutex);
        if (!userData.message_queue.empty())
        {
            struct mqttMessgae message;
            {
                message = userData.message_queue.front();

                // Writting the payload of each topics into the struct's variables
                std::cout << "\nNew MQTT Message reveiced." << std::endl;
                std::cout << "  Topic Name: " << message.topic << std::endl;
                std::cout << "  Topic Size: " << message.payloadlen << std::endl;
                std::cout << std::endl;
                userData.message_queue.pop();
            }
            // Handle message
            if (message.topic == userData.freqTopic && update_receiver == false)
            {
                // Parse the JSON string
                nlohmann::json json = nlohmann::json::parse(message.payload);
                // JSON comes in a string format, then convert it to float (std::stof) to not lose decimals when changing the unit to Hz,
                // translate it to int number (static_cast<int>) bc the receiver only accepts integer number and finally return it as a string (std::to_string).
                std::string freqValue = json["frequencies"][userData.region]["current"]["value"];
                freqInfo = std::to_string(static_cast<int>(std::stof(freqValue) * 1000000)); // in Hertz

                update_receiver = true;
            }
            else if (message.topic == userData.keyTopic)
            {
                // Parse the JSON string
                nlohmann::json json = nlohmann::json::parse(message.payload);
                keyInfo = json["dynamickeys"]["current"]["value"]; // It is already an string

                // send key
                ePPL_ReturnStatus ePPLRet;

                std::cout << "Authentication with Dynamic Key ... ";
                ePPLRet = PPL_SendDynamicKey(keyInfo.data(), keyInfo.length());
                if (ePPLRet != ePPL_Success)
                {
                    std::cerr << "FAILED. \n"
                              << std::endl;
                    std::cout << "PPL Authentication error: " << ePPLRet << std::endl; // Invlid lenght or format (!)
                    std::cout << "  - Used Key:   " << keyInfo << std::endl;
                    std::cout << "  - Key lenght: " << keyInfo.length() << std::endl;
                }
                else
                {
                    std::cout << "SUCCESS. \n"
                              << std::endl;
                    std::cout << "  - Used Key:   " << keyInfo << std::endl;
                    std::cout << "  - Key lenght: " << keyInfo.length() << std::endl;
                    std::cout << std::endl;
                }
            }
            else if (message.topic == userData.corrTopic || message.topic == userData.nodeTopic)
            {
                std::vector<uint8_t> mqtt_data = std::vector<uint8_t>(message.payload.begin(), message.payload.end());
                std::array<uint8_t, PPL_MAX_RTCM_BUFFER> rtcm_buffer;
                uint32_t rtcm_size;
                if (options.SPARTN_Logging != "none")
                    SPARTN_file_Ip.write((char *)mqtt_data.data(), mqtt_data.size()).flush();

                ePPL_ReturnStatus ePPLRet = PPL_SendSpartn(mqtt_data.data(), mqtt_data.size());
                if ((ePPLRet) == ePPL_Success)
                {
                    ePPL_ReturnStatus ePPLRet = PPL_GetRTCMOutput(rtcm_buffer.data(), PPL_MAX_RTCM_BUFFER, &rtcm_size);
                    if (rtcm_size>0)
                    {
                        std::unique_lock<std::mutex> lock(rtcm_queue_mutex);
                        rtcm_queue.push(std::vector<uint8_t>(rtcm_buffer.begin(), rtcm_buffer.begin() + rtcm_size));
                        lock.unlock();
                        cv_rtcm.notify_one();
                    }

                }
                else
                {
                    std::cout << "FAILED TO SEND IP DATA:  " << ePPLRet << std::endl;
                }
            }
            else if (message.topic == userData.tileTopic)
            {
                    // Parse Payload to get all the node available in the tile
                    nlohmann::json json = nlohmann::json::parse(message.payload);
                    this->nodeprefix = json["nodeprefix"];

                    // Replace the previous node with new ones
                    this->tile_dict.clear();
                    for(int i = 0 ; i < json["nodes"].size();i++){
                        this->tile_dict.push_back(json["nodes"][i]);
                    }
                    // Check if the endpoint has change
                    if (json["endpoint"] != userData.mqttServer){
                        
                        //Search for closest node
                        userData.nodeTopic = new_Node_Topic();
                        // Change the mqtt end point by disconnection the current one and connecting it to the new one
                        std::cout << "\nSwitching MQTT Server to : " <<json["endpoint"]<<std::endl;
                        int ret = switch_mqtt_server(json["endpoint"]);
                        if(ret != ssnppl_error::SUCCESS){
                            std::cout << "Failed to switch MQTT Server" <<std::endl;
                        }                        
                    }else{
                        process_new_position();
                    }

            }
        }
    }

    // Handle GGA and Ephemeris
    {
        std::lock_guard<std::mutex> mutex(ephemeris_gga_mutex);
        if (!ephemeris_gga_queue.empty())
        {
            std::vector<uint8_t> msg = ephemeris_gga_queue.front();
            ePPL_ReturnStatus ePPLRet = PPL_SendRcvrData(msg.data(), msg.size());
            if (ePPLRet != ePPL_Success)
            {
                std::cout << "FAILED TO SEND RCVR DATA" << std::endl;
            }
            else
            {
                std::cout << "Ephemeris Received. Size:" << msg.size() << std::endl;
            }
            ephemeris_gga_queue.pop();
            
            // parse msg to found lat and lon 
            if (userData.localized &&( options.mode == "Dual" || options.mode == "Ip") ){
                 std::vector<std::string> parsedGGA = split(std::string(msg.begin(),msg.end()),',');
                if(parsedGGA.at(0) == "$GPGGA" )
            {

                //****** Check if the GPGGA is not empty (no PVT) ********
                if (parsedGGA.at(3) == ""){
                    return;
                }

                float new_latitude = NMEAToDecimal(parsedGGA.at(2), parsedGGA.at(3));
                float new_longitude = NMEAToDecimal(parsedGGA.at(4),parsedGGA.at(5));
                if( abs(latitude - new_latitude) > latitude_threshold || abs(longitude - new_longitude) > longitude_threshold){
                    std::cout<<"New position  : lat : " << new_latitude << " / lon : "<< new_longitude <<std::endl;
                     latitude = new_latitude ;
                     longitude = new_longitude;
                     latitude_threshold = latitude;
                     longitude_threshold = latitude_threshold * cos(radians(new_latitude));
                     process_new_position();
                 }
            }
            }
        }
    }

    // Handle Incoming lband
    if (options.mode == "Lb" || options.mode == "Dual")
    {
        std::lock_guard<std::mutex> mutex(lband_queue_mutex);
        if (!lband_queue.empty())
        {
            std::vector<uint8_t> msg = lband_queue.front();
            if (options.SPARTN_Logging != "none")
                SPARTN_file_Lb.write((char *)msg.data(), msg.size()).flush();

            ePPL_ReturnStatus ePPLRet = PPL_SendAuxSpartn(msg.data(), msg.size());
            if (ePPLRet != ePPL_Success)
            {
                std::cout << "FAILED TO SEND LBAND DATA:  " << ePPLRet << std::endl;
            }
            else
            {
                std::array<uint8_t, PPL_MAX_RTCM_BUFFER> rtcm_buffer;
                uint32_t rtcm_size = 0;

                PPL_GetRTCMOutput(rtcm_buffer.data(), PPL_MAX_RTCM_BUFFER, &rtcm_size);

                if (rtcm_size>0)
                {
                    std::unique_lock<std::mutex> mutex(rtcm_queue_mutex);
                    rtcm_queue.push(std::vector<uint8_t>(rtcm_buffer.begin(), rtcm_buffer.begin() + rtcm_size));
                    mutex.unlock();
                    cv_rtcm.notify_one();
                }
            }
            lband_queue.pop();
        }
    }
}

ssnppl_error Ssnppl_demonstrator::init_ppl()
{

    std::cout << "\nInitializing Point Perfect Library ... ";

    ePPL_ReturnStatus ePPLRet;

    // Initialize PointPerfect Library
    if (options.mode == "Ip")
    {
        ePPLRet = PPL_Initialize(PPL_CFG_ENABLE_IP_CHANNEL);
    }
    else if (options.mode == "Lb")
    {
        ePPLRet = PPL_Initialize(PPL_CFG_ENABLE_AUX_CHANNEL);
    }
    else if (options.mode == "Dual")
    {
        ePPLRet = PPL_Initialize(PPL_CFG_ENABLE_IP_CHANNEL | PPL_CFG_ENABLE_AUX_CHANNEL);
    }

    // Tile level for Localized distribution
        if (options.tile_level >= 2 )
        {
            this->tile_level = 2;
        }
        else if (options.tile_level <= 0)
        {
            this->tile_level = 0;
        }
        else
        {
            this->tile_level = options.tile_level;
        }

    if (ePPLRet != ePPL_Success)
    {
        std::cerr << "FAILED. \n"
                  << std::endl;
        return PPL_FAILED;
    }
    else
    {
        std::cout << "SUCCESS. \n"
                  << std::endl;
        return SUCCESS;
    }
}

void Ssnppl_demonstrator::init_receiver()
{
    if (options.send_cmds == true)
    {

        std::vector<std::string> cmds;

        std::cout << "Starting Receiver configuration with commands ...\n"
                  << std::endl;

        // Default Config Copy
        if (options.reset_default == true)
        {

            std::cout << "Setting Receiver to Factory Default Config. Sending => eccf, RxDefault, Current" << std::endl;
            main_channel.sync_write("SSSSSSSSSSSSSSSSSSSSSSS\x0D");
            std::this_thread::sleep_for(std::chrono::seconds(2));

            main_channel.sync_write("eccf, RxDefault, Current\x0D");
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }

        // Basic commands
        cmds.push_back("sdio, " + options.receiver_main_port + ", auto, RTCMv3+NMEA\x0D");
        cmds.push_back("sr3o, " + options.receiver_main_port + ", RTCM1019+RTCM1020+RTCM1042+RTCM1046\x0D");
        cmds.push_back("sno, Stream1, " + options.receiver_main_port + ", GGA+ZDA, sec1\x0D");

        if (options.logging != "none")
        {

            // Select SBF Logginf File Name
            cmds.push_back("sfn, DSK1, FileName, " + options.logging + "\x0D");
            if (options.SBF_Logging_Config != "none")
            { // Get Messages and Interval
                std::vector<std::string> SBF_Logging_Parameters = split(options.SBF_Logging_Config, '@');
                std::string SBF_Logging_Messages = SBF_Logging_Parameters[0];
                std::string SBF_Logging_Interval = SBF_Logging_Parameters[1];

                // SBF Stream configuration
                cmds.push_back("sso, Stream3, DSK1, " + SBF_Logging_Messages + ", " + SBF_Logging_Interval + "\x0D");
            }

            if (options.NMEA_Logging_Config != "none")
            {
                std::vector<std::string> NMEA_Logging_Parameters = split(options.NMEA_Logging_Config, '@');
                std::string NMEA_Logging_Messages = NMEA_Logging_Parameters[0];
                std::string NMEA_Logging_Interval = NMEA_Logging_Parameters[1];

                // NMEA Stream configuration
                cmds.push_back("sso, Stream3, DSK1, " + NMEA_Logging_Messages + ", " + NMEA_Logging_Interval + "\x0D");
            }
        }

        // Send all configuration commands to receiver
        // Enter command mode
        main_channel.sync_write("SSSSSSSSSSSSSSSSSSSSSSS\x0D");
        std::this_thread::sleep_for(std::chrono::seconds(2));

        // Send commands
        size_t cmds_size = cmds.size();
        std::cout << "\nTotal configuration commands to send: " + std::to_string(cmds_size) + "\n"
                  << std::endl;
        for (int i = 0; i < cmds_size; i++)
        {
            std::cout << "Sending configuration commands: " + std::to_string(i + 1) + "/" + std::to_string(cmds_size) << " => " << cmds[i] << std::endl;
            main_channel.sync_write(cmds[i]);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

    }

    read_ephemeris_gga_data_thread = std::thread(&Ssnppl_demonstrator::read_ephemeris_gga_data, this);
    write_rtcm_thread = std::thread(&Ssnppl_demonstrator::write_rtcm, this);

    if (options.mode == "Lb" || options.mode == "Dual")
        read_lband_data_thread = std::thread(&Ssnppl_demonstrator::read_lband_data, this);
}

void Ssnppl_demonstrator::write_rtcm()
{
    if (options.mode != "Ip")
    {
        // Wait to have frq before entering loop if LBand
        while (!update_receiver)
            std::this_thread::yield();
    }

    while (thread_running)
    {
        if (update_receiver)
        {
            std::cout << "New frq, update receiver" << std::endl;
            main_channel.sync_write("SSSSSSSSSSSSSSSSSSSSSSS\x0D");
            std::this_thread::sleep_for(std::chrono::seconds(2));
            std::string receiver_lband_port = "USB2";

            main_channel.sync_write("slbb, User1, " + freqInfo + ", baud2400, , , Enabled\x0D"); // 1545260000
            std::this_thread::sleep_for(std::chrono::seconds(1));

            main_channel.sync_write("slsm, manual, Inmarsat, User1, User2\x0D");
            std::this_thread::sleep_for(std::chrono::seconds(1));
            main_channel.sync_write("slcs, 5555, 6959\x0D");
            std::this_thread::sleep_for(std::chrono::seconds(1));
            main_channel.sync_write("sdio, " + receiver_lband_port + ", none, LBandBeam1\x0D");
            std::this_thread::sleep_for(std::chrono::seconds(1));
            update_receiver = false;
        }

        {
            std::unique_lock<std::mutex> mutex(rtcm_queue_mutex);

            // wait for new rtcm message to send
            cv_rtcm.wait_for(mutex, std::chrono::seconds(1), [this]
                         { return !rtcm_queue.empty(); });

            if(!rtcm_queue.empty())
            {
                auto message = rtcm_queue.front();

                std::vector<int> rtcm_id = identifyRTCM3MessageIDs(message.data(), message.size());
                std::cout << "Sending RTCM3 messages";
                if (rtcm_id.size() > 0)
                {
                    std::cout << ", id = ";
                    for (int &id : rtcm_id)
                        std::cout << id << " ";
                }

                std::cout << std::endl;

                main_channel.sync_write(message.data(), message.size());
                rtcm_queue.pop();

            }
        }
    }
}

void Ssnppl_demonstrator::read_lband_data()
{
    while (thread_running)
    {
        size_t size = lband_channel.sync_read();

        uint8_t *buff = lband_channel.getSyncBuffer();

        if (!is_empty(buff, size))
        {
            std::lock_guard<std::mutex> mutex(lband_queue_mutex);

            std::vector<uint8_t> lband_vector(buff, buff + size);
            lband_queue.push(lband_vector);
        }

        lband_channel.clearSyncBuffer();
        cv_incoming_data.notify_all();
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
}

void Ssnppl_demonstrator::read_ephemeris_gga_data()
{
    while (thread_running)
    {
        size_t size = main_channel.sync_read();

        uint8_t *buff = main_channel.getSyncBuffer();

        if (!is_empty(buff, size))
        {
            std::lock_guard<std::mutex> mutex(ephemeris_gga_mutex);

            std::vector<uint8_t> ephemeris_gga_vector(buff, buff + size);
            ephemeris_gga_queue.push(ephemeris_gga_vector);
        }

        main_channel.clearSyncBuffer();
        cv_incoming_data.notify_all();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

ssnppl_error Ssnppl_demonstrator::dispatch()
{
    auto start = std::chrono::high_resolution_clock::now();
    
    while (options.timer == 0 || std::chrono::high_resolution_clock::now() - start <= std::chrono::seconds(options.timer))
    {
        handle_data();
    }

    return ssnppl_error::SUCCESS;
}

void Ssnppl_demonstrator::init_SPARTN_LOG()
{
    // Set SPARTN Loggin, if enabled
    if (options.SPARTN_Logging != "none")
    {

        // Depending on the program logic mode, open file(s)
        if (options.mode == "Ip")
            SPARTN_file_Ip.open(options.SPARTN_Logging + "_Ip.bin", std::ios::binary);
        else if (options.mode == "Lb")
            SPARTN_file_Lb.open(options.SPARTN_Logging + "_Lb.bin", std::ios::binary);
        else if (options.mode == "Dual")
        {
            SPARTN_file_Ip.open(options.SPARTN_Logging + "_Ip.bin", std::ios::binary);
            SPARTN_file_Lb.open(options.SPARTN_Logging + "_Lb.bin", std::ios::binary);
        }
    }
}

Ssnppl_demonstrator::~Ssnppl_demonstrator()
{
    thread_running = false;

    // Stop MQTT
    if (mosq_client != nullptr)
    {
        mosquitto_disconnect(mosq_client);
        mosquitto_loop_stop(mosq_client, true);
    }

    if (SPARTN_file_Ip.is_open()) SPARTN_file_Ip.close();
    if (SPARTN_file_Lb.is_open()) SPARTN_file_Lb.close();


    read_ephemeris_gga_data_thread.join();
    read_lband_data_thread.join();
    write_rtcm_thread.join();
}


// Localized Distribution Functions


void Ssnppl_demonstrator::process_new_position () noexcept
{
    // Search for current tile 
    std::string new_tile_topic = new_Tile_Topic();
    if (new_tile_topic != userData.tileTopic)
    {   //Current tile changed 
        // Unsubscribe from current tile topic 
        if (userData.tileTopic != ""){
            int result = mosquitto_unsubscribe(mosq_client,NULL,userData.tileTopic.c_str());
            if (result != MOSQ_ERR_SUCCESS) {
                    std::cerr << "\nError unsubscribing to " << userData.tileTopic.c_str() << " topic.\n" << std::endl;

                } else { 
                    std::cout << "unsubscribed from topic: " << userData.tileTopic.c_str() << std::endl;
                }
        }
        // Subscribe to new tile topic
        userData.tileTopic = new_tile_topic;
        int result = mosquitto_subscribe(mosq_client,NULL,userData.tileTopic.c_str(),userData.tileQoS) ;
        if (result != MOSQ_ERR_SUCCESS) {
                std::cerr << "\nError subscribing to " << userData.tileTopic.c_str() << " topic.\n" << std::endl;

            } else { 
                std::cout << "Subscribed to topic: " << userData.tileTopic.c_str() << std::endl;
            }
    }else {
        // Check if need to change Node topic
        process_new_node();
    }
    
}

void Ssnppl_demonstrator::process_new_node() noexcept 
{
    // Search for closest node 
    std::string new_node_topic = new_Node_Topic();
    if (new_node_topic != this->userData.nodeTopic)
    {
        //New node topic found
        // Unsubscribe from current node topic 
        if (this->userData.nodeTopic !=""){
            int result = mosquitto_unsubscribe(mosq_client,NULL,userData.nodeTopic.c_str());
            if (result != MOSQ_ERR_SUCCESS) {
                std::cerr << "\nError unsubscribing to " << userData.nodeTopic.c_str() << " topic.\n" << std::endl;

            } else { 
                std::cout << "unsubscribed from topic: " << userData.nodeTopic.c_str() << std::endl;
            }
        }
        // Subscribe to new node topic
        userData.nodeTopic = new_node_topic;
        int result = mosquitto_subscribe(mosq_client,NULL,userData.nodeTopic.c_str(),userData.nodeQoS) ;
        if (result != MOSQ_ERR_SUCCESS) {
                std::cerr << "\nError subscribing to " << userData.nodeTopic.c_str() << " topic.\n" << std::endl;

            } else { 
                std::cout << "Subscribed to topic: " << userData.nodeTopic.c_str() << std::endl;
            }
    }
}

std::string Ssnppl_demonstrator::new_Node_Topic() noexcept
{
    
    float min_dist_scaled = std::numeric_limits<float>::max();
    float node_lat , node_lon ;
    char node_ns , node_ew ;
    float dist ;
    std::string result ;
    for( int i = 0 ; i < tile_dict.size() ; i++){
        // Get the node information
        node_ns = tile_dict.at(i).at(0);
        node_lat = stof(tile_dict.at(i).substr(1,4))/100;
        node_ew = tile_dict.at(i).at(5);
        node_lon = stof(tile_dict.at(i).substr(6,11)) /100;

        if (node_ns == 'S'){
            node_lat = -node_lat;
        }
        if (node_ew == 'W'){
            node_lon = -node_lon;
        }
        dist = distanceBetweenLocations(latitude,longitude,node_lat,node_lon);

        if (dist < min_dist_scaled) {
            min_dist_scaled = dist;
            result = tile_dict.at(i);
        }
    }
    return this->nodeprefix + result;
    
}
std::string Ssnppl_demonstrator::new_Tile_Topic () noexcept
{
    char latitude_direction = (latitude < 0) ? 'S' : 'N' ;
    char longitude_direction = (longitude < 0) ? 'W' : 'E' ;
    float scale =(float)  (1000 >> (tile_level & 3)) / 100.0;
    
    float tileLat  = (floor(latitude / scale) * scale) + (scale/2);
    float tileLon  = (floor(longitude / scale) * scale) + (scale/2);

    int final_tile_Lat = abs(round(tileLat * 100));
    int final_tile_Lon = abs(round(tileLon * 100));

    std::ostringstream result ;
    result << "pp/ip/L" << tile_level << latitude_direction ;
    result << std::setw(4) << std::setfill('0') << final_tile_Lat;
    result << longitude_direction;
    result << std::setw(5) << std::setfill('0') << final_tile_Lon ;
    result << "/dict";
    return result.str(); 
}
