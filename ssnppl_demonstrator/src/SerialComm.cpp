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

#include "SerialComm.hpp"

/*Opening a serial port with the specified device path and baud rate.*/
void SerialPort::open_serial_port (std::string device_path, unsigned int baud_rate)
{   

    try
    {
        serial_port.reset();
        serial_port = serial_port_ptr(new boost::asio::serial_port(io_service));
        serial_port->open(device_path, error);

        if (error) 
        {
            std::cout << "There was an error trying to initialize the serial port." << std::endl;
            std::cout << "error.message() >> " << error.message().c_str() << std::endl;
            throw -3;
        }

        // set options
        serial_port->set_option(boost::asio::serial_port_base::baud_rate(baud_rate));
        serial_port->set_option(boost::asio::serial_port_base::character_size(8));
        serial_port->set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));
        serial_port->set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
        serial_port->set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none));    

    }
    catch (int error)
    {
        std::cout << "error = " << error;
        throw -1;
    }
    catch (const std::exception &e)
    {
        std::cout << "e.what() = " << e.what() << std::endl;
        throw -2;
    }

    std::cout << "The " << device_path << " serial port was opened correctly." << std::endl;  

    return;
}

/*  The async_read_some function initiates an asynchronous read operation on the serial port. 
    This function returns immediately, allowing the program to continue executing other tasks while the read operation is in progress. 
    When data is received from the serial port, the data_received function is called to process the received data. */
void SerialPort::async_read_some (void)
{
    /*  
        boost::mutex::scoped_lock lock (mutex);

        Creates a scoped_lock object named lock that acquires the mutex object when it is constructed. 
        This ensures that only one thread can execute the code protected by the mutex at a time. 
        When the lock object goes out of scope, the mutex is automatically released, allowing other threads to acquire it.
    */
    boost::mutex::scoped_lock lock (mutex); // prevent multiple threads

    std::cout << "Asynchronous reading started." << std::endl; 

    try
    {
        if (!serial_port->is_open())
        {
            std::cout << "The serial port is closed when reading asynchronously. Throwing exception -2." << std::endl; 
            throw -2;
        }

        serial_port->async_read_some(
            boost::asio::buffer(read_buffer, MAX_RCVR_DATA),
            boost::bind(
                &SerialPort::data_received,
                this, boost::asio::placeholders::error, 
                boost::asio::placeholders::bytes_transferred
            )
        );
        
        /* 
            boost::thread t(boost::bind(&boost::asio::io_service::run, &io_service));

            creates a new thread of execution that runs the boost::asio::io_service::run function. 
            This function is used to start the event loop of the io_service object, 
            which is responsible for running asynchronous operations. 

            The boost::bind function is used to bind the &io_service argument to the run function. 
            This creates a function object that can be passed as an argument to the boost::thread constructor. 
            The boost::thread constructor creates a new thread of execution that runs the run function with the io_service object as an argument.   
        */
        boost::thread t(boost::bind(&boost::asio::io_service::run, &io_service));
    }
    catch (const std::exception &e)
    {
        std::cout << "e.what() = " << e.what();
        throw -1;
    }

    //std::cout << "Asynchronous reading ended." << std::endl; 

    return;
}

/*  Once the data_received function has finished processing the received data, 
    it calls the async_read_some function AGAIN to initiate another read operation. 
    This cycle continues indefinitely, allowing the program to continuously read data 
    from the serial port and process it as it becomes available. */
void SerialPort::data_received(const boost::system::error_code& error, size_t bytes_transferred)
{
       /*  
        boost::mutex::scoped_lock lock (mutex);

        Creates a scoped_lock object named lock that acquires the mutex object when it is constructed. 
        This ensures that only one thread can execute the code protected by the mutex at a time. 
        When the lock object goes out of scope, the mutex is automatically released, allowing other threads to acquire it.
    */
    boost::mutex::scoped_lock lock (mutex); // prevent multiple threads

    //std::cout << "Asynchronous callback started." << std::endl; 

    try
    {
        // determine whether the serial_port object has been initialized or if it is in a valid state and check if serial port is opened.
        if (serial_port.get() == NULL || !serial_port->is_open())
        {
            std::cout << "Serial port is not open" << std::endl; 
            throw -2;
        }
        if (error) 
        {            
            std::cout << "There was an error while checking the serial port." << std::endl;
            std::cout << "error.message() >> " << error.message().c_str() << std::endl;
            throw -3;
        }  
                 
        for (unsigned int i = 0; i < bytes_transferred; ++i) {
            serial_read_data += read_buffer[i];
        }     

        //std::cout << "Bytes_transferred: \n" << bytes_transferred << std::endl;
        //std::cout << "\nSerial Read Data:\n" << serial_read_data  << std::endl;

        serial_data_read_complete = true;
    }
    catch (const std::exception &e)
    {
        std::cout << "e.what() = " << e.what();
        throw -1;
    }

    // prevent io_service from returning due to lack of work    
    serial_port->async_read_some(
        boost::asio::buffer(read_buffer, MAX_RCVR_DATA),
        boost::bind(
            &SerialPort::data_received,
            this, boost::asio::placeholders::error, 
            boost::asio::placeholders::bytes_transferred
        )
    );

    return;
}

size_t SerialPort::sync_read()
{

    boost::mutex::scoped_lock lock (mutex); // prevent multiple threads

    // Read data from the serial port into the read_sync_buffer array
    size_t bytes_transferred = serial_port->read_some(boost::asio::buffer(read_sync_buffer));

    /*  Copy the data from the read_sync_buffer array into the serial_read_data string.

        For example, if read_sync_buffer is an array of size 256 and bytes_transferred is 5, 
        then read_sync_buffer + bytes_transferred will be a pointer to the element at the 5th 
        position within the array (i.e., read_sync_buffer[4]). This means that only the first 5 
        elements of the read_sync_buffer array will be copied into the serial_read_data string. */
    
    serial_read_data.assign(read_sync_buffer, read_sync_buffer + bytes_transferred);
    return bytes_transferred;

}

void SerialPort::sync_write(const std::string& data)
{
    boost::mutex::scoped_lock lock (mutex); // prevent multiple threads

    // Write the data to the serial port
    boost::asio::write(*serial_port, boost::asio::buffer(data));

    // Wait 1 second
    //std::this_thread::sleep_for(std::chrono::seconds(1));
}

void SerialPort::sync_write(const uint8_t *data, size_t size)
{
    boost::mutex::scoped_lock lock (mutex); // prevent multiple threads

    boost::asio::write(*serial_port, boost::asio::buffer(data, size));
}

std::string SerialPort::findUsedPort(const std::string& str)
{

    int tries = 10;

    static const std::vector<std::string> receiverPortList = {  "COM1", "COM2", "COM3", "COM4", 
                                                                "USB1", "USB2", "IP10", "IP11", 
                                                                "IP12", "IP13", "IP14", "IP15", 
                                                                "IP16", "IP17", "NTR1", "NTR2", 
                                                                "NTR3", "NTR4", "IPS1", "IPS2", 
                                                                "IPS3", "IPS4", "IPS5", "IPR1", 
                                                                "IPR2", "IPR3", "IPR4", "IPR5" };

    // Loop for search the used Port     
    for(int i = 0; i <= tries; i++)
    {
        for (const auto& seq : receiverPortList)
        {

            size_t pos = str.find(seq);
            if (pos != std::string::npos)
            {
                std::cout << "\nReceiver port: " << str.substr(pos, seq.size()) << " found.\n" << std::endl;
                used_port_found = true;
                return str.substr(pos, seq.size());
            }
        }
    }

    return "";
}

void SerialPort::setCmdInputMode()
{
    // Write the data to the serial port
    boost::asio::write(*serial_port, boost::asio::buffer("\x0DSSSSSSSSSSSSSSSSSSS\x0D\x0D"));

    // Wait 1 second
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void SerialPort::setDefaultConfig()
{
    // Write the data to the serial port
    boost::asio::write(*serial_port, boost::asio::buffer("eccf, RxDefault, current\x0D"));

    // Wait 1 second
    std::this_thread::sleep_for(std::chrono::seconds(1));
}