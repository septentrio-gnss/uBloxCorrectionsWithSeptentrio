/*

g++ -std=c++11 -o ../build/gluecode gluecode.cpp \
-I../include/pointPerfect/PPL_64_Bit/inc/ \
-I../include/mqtt/ \
-I../include/serialComm/ \
-L../include/pointPerfect/PPL_64_Bit/lib \
-lmosquitto \
-lpointperfect \
-pthread \
-lboost_system \
-lboost_thread \
-lboost_program_options

../include/serialComm/SerialComm.cpp \

../build/gluecode --mode Ip --main_comm USB --main_config /dev/ttyACM0@115200 --client_id 67bfb9fd-3656-4ebe-a9a9-e0ac2c371cba --echo true --reset_default true 
../build/gluecode --mode Lb --main_comm USB --main_config /dev/ttyACM0@115200 --client_id 67bfb9fd-3656-4ebe-a9a9-e0ac2c371cba --echo true --reset_default true --lband_comm USB --lband_config /dev/ttyACM1@115200 

*/

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

// Standard Libraries and some Support methods for the gluecode
#include "../include/gluecode/gluecode.hpp"

// MQTT
#include "../include/mqtt/mqtt.hpp"

// SERIAL
#include "../include/serialComm/SerialComm.hpp" 

// PPL
#include "../include/pointPerfect/PPL_64_Bit/inc/PPL_PublicInterface.h" // PointPerfect Library

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

    // This function shows all option values of the program, store them into a variables of a struct
    // and returns the struct
    ProgramOptions options = ParseProgramOptions(argc, argv);

    showOptions(options);

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
    echo("\n*** COMMUNICATION CHANNELS CONFIGURATION ********************************\n\n", options.echo);

    // get Main Channel params
    std::vector<std::string> main_usb_parameters = split(options.main_config, '@');
    std::string main_usb_port = main_usb_parameters[0];
    int main_usb_baudrate = std::stoi(main_usb_parameters[1]);
    echo("Opening main channel serial port ...\n\n", options.echo);

    // Create main comm channel object
    SerialPort main_channel(main_usb_port, main_usb_baudrate);
    main_channel.checkIfOpen();

    if (options.main_comm == "USB") options.receiver_main_port = "USB1";

    SerialPort* lband_channel_ptr;
    if (options.lband_comm != "none"){
        // get LBand Channel params
        std::vector<std::string> lband_usb_parameters = split(options.lband_config, '@');
        std::string lband_usb_port = lband_usb_parameters[0];
        int lband_usb_baudrate = std::stoi(lband_usb_parameters[1]);
        echo("Opening aux channel serial port ...\n", options.echo);

        // Create lband comm channel object
        lband_channel_ptr = new SerialPort(lband_usb_port, lband_usb_baudrate);
        lband_channel_ptr->checkIfOpen();
        if (options.lband_comm == "USB") options.receiver_lband_port = "USB2";
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
    
    echo("\n*** MQTT CLIENT CONFIGURATION *******************************************\n\n", options.echo);

    // Auth files path information
    const std::string caFile = "../auth/AmazonRootCA1.pem";
    const std::string certFile = "../auth/device-" + options.client_id + "-pp-cert.crt";
    const std::string keyFile = "../auth/device-" + options.client_id + "-pp-key.pem";

    // Initialize Moaquitto library.
    mosquitto_lib_init();
   
    // Create MQTT Client.
    echo("Creating MQTT Client.\n", options.echo);
    struct mosquitto *mosq_client = mosquitto_new(options.client_id.c_str(), true, NULL);

    // Check for errors.
    if (!mosq_client) {
        std::cerr << "Failed to create new Mosquitto client" << std::endl;
        return 1;
    }

    // Configure MQTT V5 version.
    mosquitto_int_option(mosq_client, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);

    // Authentication
    int ret = mosquitto_tls_set(mosq_client, caFile.c_str(), "./" , certFile.c_str(), keyFile.c_str(), NULL);
    if (ret != MOSQ_ERR_SUCCESS) {
        std::cerr << "Failed AUTH to MQTT broker: " << mosquitto_strerror(ret) << std::endl;
        return 1;
    }

    // Create and initialize struct to bind it to userdata object in MQTT Callbacks.
    struct UserData UserData;

    // Set the program logic mode
    UserData.corrections_mode = options.mode;
    UserData.region = options.region;

    // Setting the callbacks for the MQTT Client
    echo("Setting the callbacks for the MQTT Client.\n", options.echo);
    mosquitto_message_callback_set(mosq_client, on_message);
    mosquitto_connect_callback_set(mosq_client, on_connect);

    // set the userdata for the client instance
    mosquitto_user_data_set(mosq_client, &UserData);

    // Establish connection to the broker  
    echo("Establish connection to the broker.\n", options.echo);
    ret = mosquitto_connect(mosq_client, options.mqtt_server.c_str(), 8883, 10);
    if (ret != MOSQ_ERR_SUCCESS) {
        std::cerr << "Failed to connect to MQTT broker: " << mosquitto_strerror(ret) << std::endl;
        return 1;
    }

    // Starting the client loop                                                               *
    echo("Starting the MQTT client loop.\n", options.echo);
    ret = mosquitto_loop_start(mosq_client);
    if (ret != MOSQ_ERR_SUCCESS) {
        std::cerr << "Failed to start main loop of Mosquitto client: " << ret << std::endl;
        return 1;
    }

    // Search for Dynamic Key
    echo("Searching for Dynamic Key.\n", options.echo);
    // Look until Dynamic Key topic arrives
    options.currentKey = waitForTopic(UserData, UserData.keyTopic);
    
    // Search for LBand Frequency, if LBand enabled
    if(options.mode != "Ip") {
        // Look until Frequency topic arrives
        echo("Searching for Frequency topic.\n", options.echo);
        options.currentFreq = waitForTopic(UserData, UserData.freqTopic);

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

    //MyQueue receiverAsyncQueue(MAX_RECEIVER_QUEUE_SIZE);

    if (options.send_cmds){

        echo("*** SENDING CONFIGURATION TO THE RECEIVER *******************************\n", options.echo);
        echo("\nStarting Receiver configuration sending commands ...\n", options.echo);

        // Default Config Copy, if enabled
        if (options.reset_default) main_channel.setDefaultConfig();

        // Prepare a vector with all receiver config cmds to send
        const std::vector<std::string> cmds = prepareConfigCmds(options);

        // Send configuration via Cmds and check every reply with a timeot
        // Start Async Reading -> (Send cmd -> Check cmd reply) x Nº of cmds -> Stop Async Read. 
        int timeOut = 10;
        main_channel.sendCmdsAndCheckReply(cmds, timeOut, options.echo);

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

    echo("*** INITIALIZING POINT PERFECT LIBRARY ... ******************************\n\n", options.echo);

    ePPL_ReturnStatus ePPLRet;

    // Initialize PointPerfect Library
    if (options.mode == "Ip") {         ePPLRet = PPL_Initialize(PPL_CFG_ENABLE_IP_CHANNEL);} 
    else if (options.mode == "Lb") {    ePPLRet = PPL_Initialize(PPL_CFG_ENABLE_AUX_CHANNEL);} 
    else if (options.mode == "Dual") {  ePPLRet = PPL_Initialize(PPL_CFG_DEFAULT_CFG);}    

    if (ePPLRet!= ePPL_Success) {
        echo("PointPerfect Library Initialization: FAILED. \n", options.echo);
        return 1;

    } else {echo("PointPerfect Library Initialization: SUCCESS. \n", options.echo);}
        
    // Dynamic Key
    char* currentDynKey = str2char(options.currentKey);
    uint8_t dynKeyLen = static_cast<uint8_t>(strlen(currentDynKey));

    // Send Dynamic Key
    echo("\n*** AUTHENTICATION WITH DYNAMIC KEY ... *********************************\n\n", options.echo);

    ePPLRet = PPL_SendDynamicKey(currentDynKey, dynKeyLen);
    if (ePPLRet!= ePPL_Success) {
        std::cout << "PPL Authentication error: FAILED.\nError status: " << ePPLRet << std::endl;
        return 1;

    } else {std::cout << "PPL Authentication error: SUCCESS.\n" << std::endl;}

    echo("  - Used Key:   " + std::string(currentDynKey) + "\n", options.echo);
    echo("  - Key lenght: " + std::to_string(dynKeyLen) +  "\n", options.echo);

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
    if(options.SPARTN_Logging != "none"){

        // Depending on the program logic mode, open file(s)
        if(options.mode == "Ip") spartn_Log_Stream.SPARTN_file_Ip.open(options.SPARTN_Logging + "_Ip.bin", std::ios::binary);
        else if(options.mode == "Lb") spartn_Log_Stream.SPARTN_file_Lb.open(options.SPARTN_Logging + "_Lb.bin", std::ios::binary);
        else if(options.mode == "Dual") {
            spartn_Log_Stream.SPARTN_file_Ip.open(options.SPARTN_Logging + "_Ip.bin", std::ios::binary);
            spartn_Log_Stream.SPARTN_file_Lb.open(options.SPARTN_Logging + "_Lb.bin", std::ios::binary);
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
    std::chrono::seconds currentTime;
    std::chrono::time_point<std::chrono::high_resolution_clock> start;

    // Start timer and assing value to duration and start, if enabled
    if (options.timer > 0){
        // Set the duration of the loop
        duration = std::chrono::seconds(options.timer);

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

    // Variables to get RTCM from PointPerfect Library
    uint8_t rtcmBuffer[PPL_MAX_RTCM_BUFFER];
    uint32_t rtcmSize = 0;

    // Epoc variables
    int epoc = 0;
    int RTCMGenerated = 0;

    echo("\n*** STARTING MAIN PROCCESSING LOOP ... **********************************\n", options.echo);

    while(true){
        // Timer Check
        if (options.timer > 0){
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
                if (options.logging != "none"){
                    std::cout << "Stoping SBF Logging ..." << std::endl;
                    
                    main_channel.setCmdInputMode();
                    main_channel.syncWriteStr("sso, Stream3, none, none, off\x0D");
                    std::this_thread::sleep_for(std::chrono::seconds(1));
    
                }   

                // Finish SPARTN Logging, if enabled
                if (options.SPARTN_Logging != "none"){
                    std::cout << "Stoping SPARTN Logging ..." << std::endl;

                    // Close the file streams
                    //spartn_Log_Stream.closeAll();
                }           
                
                break;
            }
        }

        // If there is a new Dynamic Key, change it.
        if(UserData.keyInfo != currentDynKey) {
            
            currentDynKey = str2char(UserData.keyInfo);
            dynKeyLen = static_cast<uint8_t>(strlen(currentDynKey));

            ePPLRet = PPL_SendDynamicKey(currentDynKey, dynKeyLen);
            if (ePPLRet!= ePPL_Success) {
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
        if(options.mode != "Ip" && UserData.freqInfo != options.currentFreq) { 
            // Send SSSSSS... command to put receiver's input in Command INput Mode.
            main_channel.syncWriteStr("SSSSSSSSSSSSSSSSSSSSSSS\x0D");
            std::this_thread::sleep_for(std::chrono::seconds(2));

            // Send command with new frequency.
            options.currentFreq = UserData.freqInfo;
            main_channel.syncWriteStr("slbb, User1, " + options.currentFreq + ", baud2400, , , Enabled\x0D");
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        echo("#####################################################################", options.echo);
        echo("# EPOC Nº:            " + std::to_string(epoc) + , options.echo);
        echo("# RTCM GENERATED :    " + std::to_string(RTCMGenerated) + , options.echo);

        /******************************************************************************
                                    Get and Send Ephemeris & NMEA
        *******************************************************************************/
        // Reading Ephemeris and NMEA Reading from Receiver.
        if (options.main_comm == "USB") {main_channel.syncReadReceiver();} 

        // Send Ephemeris + NMEA data coming from Receiver to PPL.
        ePPL_ReturnStatus ePPLRet = PPL_SendRcvrData(
            static_cast<const void*>(receiverSyncQueue.getNextDataVec().data()), receiverSyncQueue.getNextDataVec().size());

        if (ePPLRet != ePPL_Success) echo("FAILED TO SEND RCVR DATA\n", options.echo);
        else echo("\nEphemeris Received.\n", options.echo);
        /******************************************************************************/
        
        /******************************************************************************
                                    Get and Send LBand SPARTN
        *******************************************************************************/
        // Check if we are using LBand
        if (options.mode == "Lb" || options.mode == "Dual"){
            // Reading Raw LBand data from lband_channel.
            if (options.lband_comm == "USB") lband_channel_ptr->syncReadLband();
            //if(SPARTN_Logging != "none") spartn_Log_Stream.SPARTN_file_Lb.write((char*)&lbandData, MAX_RCVR_DATA).flush(); // y sin flush?
            
            // Send LBand data coming from Receiver to PPL.
            ePPL_ReturnStatus ePPLRet = PPL_SendAuxSpartn(
                static_cast<const void*>(lbandSyncQueue.getNextDataVec().data()), lbandSyncQueue.getNextDataVec().size());

            if (ePPLRet != ePPL_Success) echo("FAILED TO SEND LBAND DATA: " + std::to_string(ePPLRet) + "\n", options.echo);
            else echo("LBand Received.\n", options.echo);
        } 
        /******************************************************************************/

        /******************************************************************************
                                    Get and Send MQTT SPARTN
        *******************************************************************************/
        if (options.mode == "Ip" || options.mode == "Dual"){
            //if(SPARTN_Logging != "none") spartn_Log_Stream.SPARTN_file_Lb.write((char*)&mqttData, mqttData_size).flush();
            ePPLRet = PPL_SendSpartn(
                static_cast<const void*>(corrQueue.getNextDataVec().data()), corrQueue.getNextDataVec().size());

            if (ePPLRet != ePPL_Success) echo("FAILED TO SEND CORRECTIONS FROM MQTTT: " + std::to_string(ePPLRet) + "\n", options.echo);
            else echo("MQTT Corrections sended to Point Perfect Library.", options.echo);
        }
        /*******************************************************************************/

        /******************************************************************************
                                    Get and Send MQTT SPARTN
        *******************************************************************************/
        // Once we sent to PPL al the data it needs, whatever is the mode or the selected channels:
        // Get new RTCM From Point Perfect Library
        ePPLRet = PPL_GetRTCMOutput(rtcmBuffer, PPL_MAX_RTCM_BUFFER, &rtcmSize);

        if (ePPLRet != ePPL_Success) echo("RTCM FAILED.", options.echo);
        else if (ePPLRet == ePPL_Success && rtcmSize > 0) {
            echo("RTCM SUCCESS. \nRTCM Size: " + std::to_string(rtcmSize), options.echo);
            // Send data to Receiver
            main_channel.syncWriteArr(rtcmBuffer, rtcmSize);

            // Clean RTCM Buffers
            std::memset(rtcmBuffer, 0, PPL_MAX_RTCM_BUFFER);
            rtcmSize = 0;
            RTCMGenerated++;

        }       
        /*******************************************************************************/
        epoc++;
    } // End of while(true){

    return 0;
}
