/*
#include <boost/asio.hpp>
#include <boost/asio/serialPort_.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/bind/bind.hpp>
#include <boost/thread.hpp>

#include <string>
#include <cstring>
#include <atomic>
#include <iostream>
#include <queue> */

#define MAX_RCVR_DATA 150
#define MAX_QUEUE_ELEMENTS 10

#ifndef SERIALPORT_HPP
#define SERIALPORT_HPP

#include <boost/asio.hpp>
#include <chrono>
#include <boost/thread.hpp>
#include <algorithm>
#include <thread>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>

#include "../queue/queue.hpp"

extern MyQueue receiverAsyncQueue(MAX_RECEIVER_QUEUE_SIZE); // declare receiverAsyncQueue as an extern variable
extern MyQueue receiverSyncQueue(MAX_RECEIVER_QUEUE_SIZE); // declare receiverSyncQueue as an extern variable
extern MyQueue lbandSyncQueue(MAX_RECEIVER_QUEUE_SIZE); // declare receiverSyncQueue as an extern variable


class SerialPort {
    public:
        SerialPort(const std::string& portName, unsigned int baudRate) :
            ioService_(),
            serialPort_(ioService_, portName),
            baudRate_(baudRate),
            portName_(portName)
        {
            // set options
            serialPort_.set_option(boost::asio::serial_port_base::baud_rate(baudRate_));
            serialPort_.set_option(boost::asio::serial_port_base::character_size(8));
            serialPort_.set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));
            serialPort_.set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
            serialPort_.set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none));    

        }

        ~SerialPort() {serialPort_.close();}

        void syncWriteStr(const std::string& data){boost::asio::write(serialPort_, boost::asio::buffer(data));}
        void syncWriteArr(const uint8_t *data, size_t size){boost::asio::write(serialPort_, boost::asio::buffer(data, size));}
        
        void syncReadReceiver(){
            size_t bytes_transferred = boost::asio::read(serialPort_, boost::asio::buffer(syncReadBuffer_, MAX_RCVR_DATA));
            receiverSyncQueue.addData(syncReadBuffer_, bytes_transferred);
        }

        void syncReadLband(){
            size_t bytes_transferred = boost::asio::read(serialPort_, boost::asio::buffer(syncReadBufferLband_, MAX_RCVR_DATA));
            lbandSyncQueue.addData(syncReadBufferLband_, bytes_transferred);
        }

        void stopAsyncReadSome(void){serialPort_.cancel();}

        void startAsyncReadSome(void){

            boost::mutex::scoped_lock lock (mutex); // prevent multiple threads
            std::cout << portName_ << " Serial port asynchronous reading STARTED.\n" << std::endl;

            try{
                if (!serialPort_.is_open()){
                    std::cout << "The serial port is closed when reading asynchronously. Throwing exception -2." << std::endl; 
                    throw -2;
                }

                serialPort_.async_read_some(
                    boost::asio::buffer(asyncReadBuffer_, MAX_RCVR_DATA),
                    boost::bind(
                        &SerialPort::data_received,
                        this, boost::asio::placeholders::error, 
                        boost::asio::placeholders::bytes_transferred
                    )
                );

                boost::thread t(boost::bind(&boost::asio::io_service::run, &ioService_));

            }catch (const std::exception &e){
                std::cout << "e.what() = " << e.what();
                throw -1;
            }

            return;
        }

        void sendCmdsAndCheckReply(const std::vector<std::string> &cmds, const int &timeOut, const bool &echo){

            int replyFound = 0;
            size_t cmdSize = cmds.size();
            std::string asyncMsg = "";
            std::chrono::time_point<std::chrono::high_resolution_clock> start_time;

            startAsyncReadSome();
            setCmdInputMode();

            if(echo) std::cout << "Total configuration commands to send: " + std::to_string(cmdSize) + "\n" << std::endl;

            for (int i = 0; i < cmdSize; i++) {

                // Send command
                if(echo) std::cout << "Sending command: " + std::to_string(i + 1) + "/" + std::to_string(cmdSize) + " => " + cmds[i] << std::endl;
                syncWriteStr(cmds[i]);

                // Get the current time
                start_time = std::chrono::high_resolution_clock::now();   

                // Searching while loop with timeout
                while (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - start_time).count() < timeOut)  {
                    // Get incoming asynchronous data              
                    asyncMsg = receiverAsyncQueue.getNextDataStr();

                    // Try yo find the reply in the read buffer
                    if(asyncMsg.find(cmds[i]) != std::string::npos){
                        std::cout << "=>REPLY FOUND\n" << std::endl;
                        replyFound++;
                        break;
                    } 
                }
            } // end of for (int i = 0; i < cmdSize; i++) {

            stopAsyncReadSome();
            if(echo) std::cout << "Found replies " + std::to_string(replyFound) + "/" + std::to_string(cmdSize) + "\n" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2));
            return;

        } // end of sendCmdsAndCheckReply( ... )

        void setCmdInputMode() {
            syncWriteStr("\x0DSSSSSSSSSSSSSSSSSSS\x0D\x0D");
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }

        void setDefaultConfig() {
            setCmdInputMode();
            std::cout << "Setting Receiver to Factory Default Config ..." << std::endl;
            syncWriteStr("eccf, RxDefault, current\x0D");
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }

        void checkIfOpen(){
            if (!serialPort_.is_open()) {
                std::cout << portName_ + " was not successfully opened." << std::endl;
                return;
            }else {
                std::cout << portName_ + " port is successfully opened." << std::endl;
                return;  
            }                      
        }

    private:

        boost::mutex mutex;
        boost::asio::io_service ioService_;
        boost::asio::serial_port serialPort_;
        unsigned int baudRate_;
        std::string portName_;

        uint8_t asyncReadBuffer_[MAX_RCVR_DATA];
        uint8_t syncReadBuffer_[MAX_RCVR_DATA];
        uint8_t syncReadBufferLband_[MAX_RCVR_DATA];

        void data_received(const boost::system::error_code& error, size_t bytes_transferred){
            
            boost::mutex::scoped_lock lock (mutex); // prevent multiple threads

            try{
                // determine whether the serialPort_ object has been initialized or if it is in a valid state and check if serial port is opened.
                if (!serialPort_.is_open()){
                    std::cout << portName_ << " Serial port is NOT open.\n" << std::endl; 
                    throw -2;
                }

                if (error == boost::asio::error::operation_aborted) {
                    std::cout << portName_ << "Serial port asynchronous read operation ABORTED.\n" << std::endl;

                } else if (error) {            
                    std::cout << "There was an error while checking the serial port." << std::endl;
                    std::cout << "error.message() >> " << error.message().c_str() << std::endl;
                    throw -3;
                }  
                        
                //for (unsigned int i = 0; i < bytes_transferred; ++i)serial_read_data += asyncReadBuffer_[i];

                receiverAsyncQueue.addData(asyncReadBuffer_, MAX_RCVR_DATA); 

            }catch (const std::exception &e){
                std::cout << "e.what() = " << e.what();
                throw -1;
            }

            // prevent io_service from returning due to lack of work    
            serialPort_.async_read_some(
                boost::asio::buffer(asyncReadBuffer_, MAX_RCVR_DATA),
                boost::bind(
                    &SerialPort::data_received,
                    this, boost::asio::placeholders::error, 
                    boost::asio::placeholders::bytes_transferred
                )
            );

            return;
        } // end data_received
}; // end class SerialPort {

#endif