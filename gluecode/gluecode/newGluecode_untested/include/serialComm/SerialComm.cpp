#include "SerialComm.hpp"

/******************************************************************************

                    OPEN AND CLOSE

******************************************************************************/

void SerialPort::open_serial_port (std::string device_path, int baud_rate) {   
    device_path_name = device_path;
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

    } catch (int error) {
        std::cout << "error = " << error;
        throw -1;

    } catch (const std::exception &e) {
        std::cout << "e.what() = " << e.what() << std::endl;
        throw -2;
    }

    std::cout << "The " << device_path << " serial port was opened correctly.\n" << std::endl;  

    return;
}

void SerialPort::close_serial_port() {
    if (serial_port && serial_port->is_open())
    {
        // Stop any pending asynchronous read operation
        stop_async_read();

        if (serial_port && serial_port->is_open()) {
            serial_port->cancel();
            stop_async_read();
            serial_port->close();

            if (mutex.try_lock()) {
                mutex.unlock();
                std::cout << "\nMutex destroyed\n." << std::endl;

            }
            else {
                // Handle the case where the mutex is already in a destroyed state
                std::cout << "Mutex is already in a destroyed state." << std::endl;
            }
        } else {
            std::cout << "Trying to close the serail port:\n    Serial port is not open." 
            << std::endl;
        }
    }
}

/******************************************************************************

                    SYNCHRONOUS

******************************************************************************/

void SerialPort::sync_write(const std::string& data)
{ boost::asio::write(*serial_port, boost::asio::buffer(data)); }

void SerialPort::sync_write(const uint8_t *data, size_t size)
{ boost::asio::write(*serial_port, boost::asio::buffer(data, size)); }

void SerialPort::sync_write_and_check_reply(const std::string& data, const int timeOut) {
    bool reply_found = false;

    // Send current command
    std::cout << data << std::endl;
    sync_write(data);

    // Get the current time
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time = std::chrono::high_resolution_clock::now();

    // Searching while loop with timeout
    while (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - start_time).count() < timeOut)  {

        // Try yo find the reply in the read buffer
        if(serial_read_data.find(data) != std::string::npos){
            std::cout << "\n  REPLY FOUND => " << data << "\n" << std::endl;
            reply_found = true;
            break;
        }
    }

    if(!reply_found) std::cout << " REPLY NOT FOUND" << std::endl;
    else reply_found = false;
    
}

void SerialPort::sync_read() {
    // Read data from the serial port into the read_sync_buffer array
    size_t bytes_transferred = boost::asio::read(*serial_port, boost::asio::buffer(read_sync_buffer));

    // Push sync buffer's data to queue
    addSyncData(read_sync_buffer);

    // Clean synchronous buffer
    std::memset(read_sync_buffer, 0, MAX_RCVR_DATA);

}

/******************************************************************************

                    ASYNCHRONOUS

******************************************************************************/

void SerialPort::stop_async_read (void){ serial_port->cancel(); }

void SerialPort::start_async_read (void) {
    boost::mutex::scoped_lock lock (mutex); // prevent multiple threads
    std::cout << device_path_name << "\nSerial port asynchronous reading started." << std::endl;

    try {
        if (!serial_port->is_open())
        {
            std::cout << "The serial port is closed when reading asynchronously. Throwing exception -2." << std::endl; 
            throw -2;
        }

        serial_port->async_read_some(
            boost::asio::buffer(read_async_buffer, MAX_RCVR_DATA),
            boost::bind(
                &SerialPort::data_received,
                this, boost::asio::placeholders::error, 
                boost::asio::placeholders::bytes_transferred
            )
        );
        
        boost::thread t(boost::bind(&boost::asio::io_service::run, &io_service));
    } catch (const std::exception &e) {
        std::cout << "e.what() = " << e.what();
        throw -1;
    }

    return;
}

void SerialPort::data_received(const boost::system::error_code& error, size_t bytes_transferred) {
    boost::mutex::scoped_lock lock (mutex); // prevent multiple threads

    try {
        // determine whether the serial_port object has been initialized or if it is in a valid state and check if serial port is opened.
        if (serial_port.get() == NULL || !serial_port->is_open())
        {
            std::cout << device_path_name << " Serial port is not open" << std::endl; 
            throw -2;
        }

        if (error == boost::asio::error::operation_aborted) {
            std::cout << device_path_name << "Serial port asynchronous read operation was canceled." << std::endl;

        } else if (error) {            
            std::cout << "There was an error while checking the serial port." << std::endl;
            std::cout << "error.message() >> " << error.message().c_str() << std::endl;
            throw -3;
        }  
        
        addAsyncData(read_async_buffer);

    } catch (const std::exception &e) {
        std::cout << "e.what() = " << e.what();
        throw -1;
    }

    // prevent io_service from returning due to lack of work    
    serial_port->async_read_some(
        boost::asio::buffer(read_async_buffer, MAX_RCVR_DATA),
        boost::bind(
            &SerialPort::data_received,
            this, boost::asio::placeholders::error, 
            boost::asio::placeholders::bytes_transferred
        )
    );

    return;
}


/******************************************************************************

                    QUEUES

******************************************************************************/

void SerialPort::addSyncData(std::uint8_t* data) {
    if (syncQueue.size() < MAX_QUEUE_ELEMENTS) {
        syncQueue.push(data);
    } else {
        // Queue is full, remove an element from the front
        syncQueue.pop();
        syncQueue.push(data);
    }
}

uint8_t* SerialPort::getNextSyncData() {
    if (!syncQueue.empty()) {
        std::uint8_t* data = syncQueue.front();
        syncQueue.pop();
        return data;
    } else { return nullptr; }
}

void SerialPort::clearSyncData() {
    while (!syncQueue.empty()) {
        std::uint8_t* data = syncQueue.front();
        syncQueue.pop();
        delete[] data;
    }
}

void SerialPort::addAsyncData(std::uint8_t* data) {
    if (asyncQueue.size() < MAX_QUEUE_ELEMENTS) {
        asyncQueue.push(data);
    } else {
        // Queue is full, remove an element from the front
        asyncQueue.pop();
        asyncQueue.push(data);
    }
}

uint8_t* SerialPort::getNextAsyncData() {
    if (!asyncQueue.empty()) {
        std::uint8_t* data = asyncQueue.front();
        asyncQueue.pop();
        return data;
    } else { return nullptr; }
}

void SerialPort::clearAsyncData() {
    while (!asyncQueue.empty()) {
        std::uint8_t* data = asyncQueue.front();
        asyncQueue.pop();
        delete[] data;
    }
}

/******************************************************************************

                    OTHERS

******************************************************************************/
std::string SerialPort::findUsedPort(const std::string& str) {

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

void SerialPort::setCmdInputMode() {
    boost::asio::write(*serial_port, boost::asio::buffer("\x0DSSSSSSSSSSSSSSSSSSS\x0D\x0D"));
    std::this_thread::sleep_for(std::chrono::seconds(2));
}

void SerialPort::setDefaultConfig() {
    boost::asio::write(*serial_port, boost::asio::buffer("\x0DSSSSSSSSSSSSSSSSSSS\x0D\x0D"));
    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::cout << "Setting Receiver to Factory Default Config." << std::endl;

    boost::asio::write(*serial_port, boost::asio::buffer("eccf, RxDefault, current\x0D"));
    std::this_thread::sleep_for(std::chrono::seconds(10));
}

void SerialPort::resetReceiverSoftConfig(std::string device_path, int baud_rate) {
    boost::asio::write(*serial_port, boost::asio::buffer("\x0DSSSSSSSSSSSSSSSSSSS\x0D\x0D"));
    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::cout << "Resetting Receiver's Firmware => erst, soft, Config" << std::endl;
    std::cout << "Waiting 50 seconds to start a connection again ... " << std::endl;

    boost::asio::write(*serial_port, boost::asio::buffer("erst, soft, Config\x0D"));

    std::this_thread::sleep_for(std::chrono::seconds(5));
    close_serial_port();
    std::this_thread::sleep_for(std::chrono::seconds(40));

    if (!serial_port->is_open()) open_serial_port(device_path, baud_rate);
    std::this_thread::sleep_for(std::chrono::seconds(5));

    for (int i = 0; i < 10; ++i) {
        
        sync_write("sdio, USB1, auto, RTCMv3+NMEA\x0D");
        //boost::asio::write(*serial_port, boost::asio::buffer("sdio, USB1, auto, RTCMv3+NMEA\x0D"));
        //boost::asio::write(*serial_port, boost::asio::buffer("\x0DSSSSSSSSSSSSSSSSSSS\x0D\x0D"));
        std::this_thread::sleep_for(std::chrono::seconds(2));

    
        std::cout << "PRE - READ\n" << std::endl;

        sync_read();
        //boost::asio::read(*serial_port, boost::asio::buffer(read_sync_buffer));
        
        std::cout << "read_sync_buffer\n" << std::endl;
        std::string str;
        
        for (uint8_t c : read_sync_buffer) {
            str += static_cast<char>(c);
        }
        
        std::cout << str << std::endl; // prints "Hello World"
        
        std::cout << "serial_read_data\n" << std::endl;
        std::cout << serial_read_data << std::endl;

    }     

}