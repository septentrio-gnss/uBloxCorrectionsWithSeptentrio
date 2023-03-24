#include <boost/asio.hpp>
#include <boost/asio/serial_port.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/bind/bind.hpp>
#include <boost/thread.hpp>

#include <string>
#include <cstring>
#include <atomic>
#include <iostream>
#include <chrono>
#include <thread>

#define MAX_RCVR_DATA 150//1014

#ifndef SERIALPORT_HPP
#define SERIALPORT_HPP

class SerialPort
{
    public:

        void open_serial_port (std::string device_path, unsigned int baud_rate);

        /*  The async_read_some and data_received functions are called in a loop, 
            with the async_read_some function being called inside the data_received function and vice versa. 
            This creates a continuous cycle of reading data from the serial port and processing the received data.*/

        void close_serial_port (void);

        void async_read_some (void);
        void sync_read();

        void sync_write(const std::string& data);
        void sync_write(const uint8_t *data, size_t size);
        
        uint8_t* getSyncBuffer() { return read_sync_buffer; };

        void clearSyncBuffer() {std::memset(read_sync_buffer, 0, MAX_RCVR_DATA);};

        void setCmdInputMode();
        void setDefaultConfig();

        std::string findUsedPort(const std::string& str);

        bool isOpen() const { return serial_port->is_open(); };

        //main buffer
        std::string serial_read_data;

        std::atomic <bool> serial_data_read_complete{false};
        std::atomic <bool> used_port_found{false};

    private:

        void data_received (const boost::system::error_code& ec, size_t bytes_transferred);

        boost::mutex mutex;
        boost::asio::io_service io_service;
        boost::system::error_code error;

        typedef boost::shared_ptr<boost::asio::serial_port> serial_port_ptr;
        serial_port_ptr serial_port;
        
        std::vector<char> async_buffer_;

        // Final read async buffer
        char read_buffer[MAX_RCVR_DATA];

        // Final read sync buffer
        uint8_t read_sync_buffer[MAX_RCVR_DATA];
};

#endif
