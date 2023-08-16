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

#define MAX_RCVR_DATA 10000

#ifndef SERIALPORT_HPP
#define SERIALPORT_HPP

class SerialPort
{
public:
    void open_serial_port(std::string device_path, unsigned int baud_rate);

    /*  The async_read_some and data_received functions are called in a loop,
        with the async_read_some function being called inside the data_received function and vice versa.
        This creates a continuous cycle of reading data from the serial port and processing the received data.*/

    void close_serial_port(void);

    void async_read_some(void);
    size_t sync_read();

    void sync_write(const std::string &data);
    void sync_write(const uint8_t *data, size_t size);

    uint8_t *getSyncBuffer() { return read_sync_buffer; };

    void clearSyncBuffer() { std::memset(read_sync_buffer, 0, MAX_RCVR_DATA); };

    void setCmdInputMode();
    void setDefaultConfig();

    std::string findUsedPort(const std::string &str);

    bool isOpen() const { return serial_port->is_open(); };

    // main buffer
    std::string serial_read_data;

    std::atomic<bool> serial_data_read_complete{false};
    std::atomic<bool> used_port_found{false};

private:
    void data_received(const boost::system::error_code &ec, size_t bytes_transferred);

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
