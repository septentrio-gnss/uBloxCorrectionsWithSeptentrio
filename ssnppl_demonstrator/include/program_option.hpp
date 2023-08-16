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