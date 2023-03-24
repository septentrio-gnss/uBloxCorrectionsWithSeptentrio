#include <boost/circular_buffer.hpp>
#include <bitset>
#include <mutex>

#define MAX_CORRECTIONS_SIZE 8000

class CircularBuffer {
    public:
        // Constructor that initializes the buffer with a given length
        CircularBuffer(size_t length) : buffer_(length), mutex_() {}

        // Constructor that initializes the buffer with a default length
        CircularBuffer() : buffer_(MAX_CORRECTIONS_SIZE), mutex_() {}

        // Function to add new elements to the buffer
        void Add(const uint8_t* data, size_t length) {
            // Lock the mutex to ensure thread safety
            std::lock_guard<std::mutex> lock(mutex_);

            // Calculate how many elements can be added without exceeding the buffer capacity
            size_t available = buffer_.capacity() - buffer_.size();
            size_t to_add = std::min(length, available);

            // Add the elements to the end of the buffer
            buffer_.insert(buffer_.end(), data, data + to_add);
        }

        // Function to remove a certain number of elements from the buffer
        void Remove(uint8_t* data, size_t length) {
            // Lock the mutex to ensure thread safety
            std::lock_guard<std::mutex> lock(mutex_);

            // Calculate how many elements can be removed without going beyond the buffer size
            size_t to_remove = std::min(length, buffer_.size());

            // Copy the elements to the provided array
            std::copy(buffer_.begin(), buffer_.begin() + to_remove, data);

            // Remove the elements from the beginning of the buffer
            buffer_.erase(buffer_.begin(), buffer_.begin() + to_remove);
        }

        // Function to print the contents of the buffer
        void Print() const {
            // Lock the mutex to ensure thread safety
            std::lock_guard<std::mutex> lock(mutex_);

            // Iterate over the elements in the buffer and print them as binary digits
            for (auto it = buffer_.begin(); it != buffer_.end(); ++it) {
                std::bitset<8> bits(static_cast<unsigned int>(*it));
                std::cout << bits << "";
            }
            
            std::cout << std::endl;
        }

    private:
        boost::circular_buffer<uint8_t> buffer_; // Boost circular buffer to store the data
        mutable std::mutex mutex_; // Mutex to ensure thread safety, declared mutable to allow locking it in const functions
};

/*
#include <boost/circular_buffer.hpp>
#include <mutex>

class CircularBuffer {
    public:
        CircularBuffer(size_t length) : buffer(length) {}
        
        void Add(const uint8_t* data, size_t size) {
            std::lock_guard<std::mutex> lock(mutex);
            buffer.insert(buffer.end(), data, data + size);
        }

        void Remove(uint8_t* data, size_t size) {
            std::lock_guard<std::mutex> lock(mutex);
            auto begin = buffer.begin();
            auto end = begin + size;
            std::copy(begin, end, data);
            buffer.erase(begin, end);
        }

        void Print() const {
            std::lock_guard<std::mutex> lock(mutex);
            for (const auto& elem : buffer) {
                std::cout << static_cast<int>(elem) << "";
            }
            std::cout << std::endl;
        }

    private:
        boost::circular_buffer<uint8_t> buffer;
        mutable std::mutex mutex;
}; */

