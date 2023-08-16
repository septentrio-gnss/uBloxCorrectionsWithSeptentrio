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

#include "program_option.hpp"


ProgramOptions ParseProgramOptions(int argc, char* argv[]) {
    ProgramOptions options;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")

        // General program Logic
        ("mode", po::value<std::string>(&options.mode)->required(),                                             "mode                       Required | Ip, Lb or Dual.")
        ("echo", po::value<bool>(&options.echo)->default_value(false),                                          "echo                       Optional | true or false.")
        ("reset_default", po::value<bool>(&options.reset_default)->default_value(false),                        "reset_default                      Optional | Set Default config.")
        ("send_cmds", po::value<bool>(&options.send_cmds)->default_value(true),                                 "send_cmds                  Optional | Sends config cmds before main processing loop if TRUE.")
        ("timer", po::value<int>(&options.timer)->default_value(0),                                             "timer                      Optional | Timer to specify how long the program will run, in secs.")

        // Logging Configuration
        ("SPARTN_Logging", po::value<std::string>(&options.SPARTN_Logging)->default_value("none"),              "SPARTN_Logging:            Optional | Introduce Spartn Logfile name.")
        ("logging", po::value<std::string>(&options.logging)->default_value("none"),                            "logging:                   Optional | Introduce SBF and/or NMEA Logfile name.")
        ("SBF_Logging_Config", po::value<std::string>(&options.SBF_Logging_Config)->default_value("none"),      "SBF_Logging_Config:        Optional | If enable_SBF_logging: [Messages@Interval].") //E.g: --SBF_logging_config Support@sec1
        ("NMEA_Logging_Config", po::value<std::string>(&options.NMEA_Logging_Config)->default_value("none"),    "NMEA_Logging_Config:       Optional | If enable_SBF_logging: [Messages@Interval].") //E.g: --NMEA_Logging_Config GGA+ZDA@sec1

        // Main Channel Config
        ("main_comm", po::value<std::string>(&options.main_comm)->required(),                                   "main_comm:                 Required | USB or Ip")
        ("main_config", po::value<std::string>(&options.main_config)->required(),                               "main_config:               Required | If USB: port@baudrate If IP: address@port")
        
        // Lband Channel Config
        ("lband_comm", po::value<std::string>(&options.lband_comm)->default_value("none"),                      "lband_comm:                Optional | USB or Ip")
        ("lband_config", po::value<std::string>(&options.lband_config)->default_value("none"),                  "lband_config:              Optional | If USB: port@baudrate If IP: address@port")

        // MQTT Config
        ("client_id", po::value<std::string>(&options.client_id)->required(),                                   "client_id:                 Required | Your client id")
        ("mqtt_server", po::value<std::string>(&options.mqtt_server)->default_value("pp.services.u-blox.com"),  "mqtt_server                Optional | By Default: pp.services.u-blox.com")
        ("region", po::value<std::string>(&options.region)->default_value("eu"),                                "region                     Optional | By Default: eu")
        ("mqtt_auth_folder", po::value<std::string>(&options.mqtt_auth_folder)->default_value("auth"),           "mqtt_auth_folder:         Optional | Path to auth folder, By default : current folder");


    po::variables_map vm;

    po::store(po::parse_command_line(argc, argv, desc), vm);
    
    if (vm.count("help")) std::cout << desc << std::endl;

    po::notify(vm);


    return options;
}

void showOptions(const ProgramOptions &options) {    
    std::cout << "##########################################################################" << std::endl;
    std::cout << "#    CURRENT PROGRAM OPTIONS:                                            #" << std::endl;
    std::cout << "##########################################################################\n" << std::endl;
    std::cout << "GENERAL OPTIONS:" << std::endl;
    std::cout << "  *mode:                  " << options.mode << std::endl;
    std::cout << "  *echo:                  ";
    if(options.echo == true) std::cout << "True" << std::endl;
    else std::cout << "False" << std::endl;

    std::cout << "  *reset_default:         ";
    if(options.reset_default== true) std::cout << "True" << std::endl;
    else std::cout << "False" << std::endl;

    std::cout << "  *send_cmds:             ";
    if(options.send_cmds == true) std::cout << "True" << std::endl;
    else std::cout << "False" << std::endl;

    std::cout << "  *timer:                 ";
    if(options.timer > 0) std::cout << options.timer << " Seconds" << std::endl;
    else std::cout << "Disabled" << std::endl;

    std::cout << "\nLOGING OPTIONS:\n" << std::endl;
    std::cout << "  *SPARTN_Logging:        " << options.SPARTN_Logging << std::endl;
    std::cout << "  *logging:               " << options.logging << std::endl;
    std::cout << "  *SBF_Logging_Config:    " << options.SBF_Logging_Config << std::endl;
    std::cout << "  *NMEA_Logging_Config:   " << options.NMEA_Logging_Config << std::endl;

    std::cout << "\nMAIN CHANNEL:\n" << std::endl;
    std::cout << "  *main_comm:             " << options.main_comm << std::endl;
    std::cout << "  *main_config:           " << options.main_config << std::endl;

    std::cout << "\nLBAND CHANNEL:\n" << std::endl;
    std::cout << "  *lband_comm:            " << options.lband_comm << std::endl;
    std::cout << "  *lband_config:          " << options.lband_config << std::endl;

    std::cout << "\nMQTT SERVER:\n" << std::endl;
    std::cout << "  *client_id:             " << options.client_id << std::endl;
    std::cout << "  *mqtt_server:           " << options.mqtt_server << std::endl;
    std::cout << "  *region:                " << options.region << std::endl;
    std::cout << "\n##########################################################################\n" << std::endl;
}  