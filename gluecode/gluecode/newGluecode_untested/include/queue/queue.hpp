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
