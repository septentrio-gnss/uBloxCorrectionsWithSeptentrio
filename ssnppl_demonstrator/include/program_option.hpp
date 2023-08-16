#ifndef __PROGRAM_OPTION__
#define __PROGRAM_OPTION__

// BOOST Libraries
#include <boost/program_options.hpp>

// General Standard Libraries 
#include <string>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <vector>
#include <cstring>
#include <csignal>
#include <fstream>

namespace po = boost::program_options;

struct ProgramOptions {
    // General program Logic
    std::string mode;
    bool echo;
    bool reset_default;
    bool send_cmds;
    int timer;

    // Logging Configuration
    std::string SPARTN_Logging;
    std::string logging;
    std::string SBF_Logging_Config;
    std::string NMEA_Logging_Config;

    // Main Channel Config
    std::string main_comm;
    std::string main_config;

    // Lband Channel Config
    std::string lband_comm;
    std::string lband_config;

    // MQTT Config
    std::string client_id;
    std::string mqtt_server;
    std::string region;

    // Comms
    std::string receiver_main_port;
    std::string receiver_lband_port;

    // Lband frequency
    std::string currentFreq;

    // Dunamic Key
    std::string currentKey;

    // Auth folder
    std::string mqtt_auth_folder;
};

ProgramOptions ParseProgramOptions(int argc, char* argv[]);

void showOptions(const ProgramOptions &options);  


#endif