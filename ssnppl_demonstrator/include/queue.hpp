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

#ifndef MYQUEUE_H
#define MYQUEUE_H

#include <queue>
#include <vector>
#include <cstdint>

#define MAX_CORR_QUEUE_SIZE 10
#define MAX_LBAND_QUEUE_SIZE 10
#define MAX_RECEIVER_QUEUE_SIZE 15

class MyQueue {
    public:
        MyQueue(std::size_t maxSize) : maxQueueSize(maxSize) {}

        void addData(std::uint8_t* data, std::size_t dataSize) {
            // Create a vector of the data
            std::vector<std::uint8_t> dataCopy(data, data + dataSize);

            // Push the vector onto the queue
            queue.push(dataCopy);

            // Remove elements from the front of the queue if necessary
            while (queue.size() > maxQueueSize) {queue.pop();}
        }

        std::vector<std::uint8_t> getNextDataVec() {
            if (!queue.empty()) {
                // Get the front element of the queue and remove it
                std::vector<std::uint8_t> dataVec = queue.front();
                queue.pop();
                return dataVec;
            }     
            // If the queue is empty, return an empty vector
            return std::vector<std::uint8_t>();
        }

        std::string getNextDataStr() {
            if (!queue.empty()) {
                // Get the front element of the queue and remove it
                std::vector<std::uint8_t> dataVec = queue.front();
                queue.pop();
                std::string dataStr(dataVec.begin(), dataVec.end());
                return dataStr;

            }     
            // If the queue is empty, return an empty vector
            return "";
        }

        void clearAll() {while(!queue.empty()) {queue.pop();}}

    private:
        std::queue<std::vector<std::uint8_t>> queue;
        std::size_t maxQueueSize;
};

#endif // MYQUEUE_H
