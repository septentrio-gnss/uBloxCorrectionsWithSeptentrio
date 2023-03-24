
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
#include <chrono>
#include <thread>
#include <fstream>

void echo(const std::string & msg, bool echo_mode) {if (echo_mode == true) std::cout << msg;}

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
};

struct SPARTN_LOG {

    std::ofstream SPARTN_file_Ip;
    std::ofstream SPARTN_file_Lb;

    void closeAll() {
        SPARTN_file_Ip.close();
        SPARTN_file_Lb.close();
    }

};

ProgramOptions ParseProgramOptions(int argc, char* argv[]) {
    ProgramOptions options;
    namespace po = boost::program_options;

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
        ("region", po::value<std::string>(&options.region)->default_value("eu"),                                "region                     Optional | By Default: eu");

    po::variables_map vm;

    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) std::cout << desc << std::endl;

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

std::vector<std::string> split(const std::string& str, char separator) {
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string item;
    while (std::getline(ss, item, separator)) { result.push_back(item); }
    return result;
}

char* str2char(const std::string& str) {
    char* charArray = new char[str.length() + 1];
    std::strcpy(charArray, str.c_str());
    return charArray;
}

std::vector<std::string> prepareConfigCmds(const ProgramOptions &options) {

    // Initialize a vector to store all cmds
    std::vector<std::string> cmds;

    // Basic commands
    cmds.push_back("sdio, " + options.receiver_main_port +", auto, RTCMv3+NMEA\x0D");
    cmds.push_back("sr3o, " + options.receiver_main_port + ", RTCM1019+RTCM1020+RTCM1042+RTCM1046\x0D");
    cmds.push_back("sno, Stream1, " + options.receiver_main_port + ", GGA+ZDA, sec1\x0D");

    // SBF Logging command, if enabled
    if (options.logging != "none"){

        cmds.push_back("sfn, DSK1, FileName, " + options.logging + "\x0D");

        if (options.SBF_Logging_Config != "none"){

            // Get Messages and Interval
            std::vector<std::string> SBF_Logging_Parameters = split(options.SBF_Logging_Config, '@');
            std::string SBF_Logging_Messages = SBF_Logging_Parameters[0];
            std::string SBF_Logging_Interval = SBF_Logging_Parameters[1];

            // SBF Stream configuration
            cmds.push_back("sso, Stream3, DSK1, " + SBF_Logging_Messages + ", " + SBF_Logging_Interval + "\x0D");
        }

        if (options.NMEA_Logging_Config != "none"){

            // Get Messages and Interval
            std::vector<std::string> NMEA_Logging_Parameters = split(options.NMEA_Logging_Config, '@');
            std::string NMEA_Logging_Messages = NMEA_Logging_Parameters[0];
            std::string NMEA_Logging_Interval = NMEA_Logging_Parameters[1];
            
            // SBF Stream configuration
            cmds.push_back("sno, Stream3, DSK1, " + NMEA_Logging_Messages + ", " + NMEA_Logging_Interval + "\x0D");
        }
    }

    // Lband TRacking commands
    if (options.lband_comm != "none" && options.mode != "Ip"){
        cmds.push_back("slbb, User1, " + options.currentFreq + ", baud2400, , , Enabled\x0D"); //1545260000
        cmds.push_back("slsm, manual, Inmarsat, User1, User2\x0D"); 
        cmds.push_back("slcs, 5555, 6959\x0D");
        cmds.push_back("sdio, " + options.receiver_lband_port + ", none, LBandBeam1\x0D");
    }

    return cmds;
}
