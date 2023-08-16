#ifndef __UTILS__
#define __UTILS__

#include <vector>

void echo(const std::string &msg, bool echo_mode)
{
  if (echo_mode == true)
    std::cout << msg;
}

std::vector<std::string> split(const std::string &str, char separator)
{
  std::vector<std::string> result;
  std::stringstream ss(str);
  std::string item;
  while (std::getline(ss, item, separator))
  {
    result.push_back(item);
  }
  return result;
}

bool is_empty(const uint8_t *arr, std::size_t size)
{
  uint8_t *zeros = new uint8_t[size](); // Allocate an array of size 'size' filled with zeros.
  int result = memcmp(arr, zeros, size);
  delete[] zeros;
  return result == 0;
}

unsigned int getbitu(const unsigned char *buff, int pos, int len)
{
  unsigned int bits = 0;
  int i;
  for (i = pos; i < pos + len; i++)
    bits = (bits << 1) + ((buff[i / 8] >> (7 - i % 8)) & 1u);
  return bits;
}


std::vector<int> identifyRTCM3MessageIDs(const uint8_t *buffer, size_t bufferSize)
{
  size_t index = 0;
  std::vector<int> res;
  while (index < bufferSize)
  {
    if (bufferSize - index < 3)
    {
      std::cerr << "Invalid buffer size. Remaining bytes are not enough for an RTCM3 message." << std::endl;
      break;
    }

    uint16_t id = getbitu(buffer + index, 24, 12);

    res.push_back(id);

    // Calculate the length of the current RTCM3 message
    uint16_t length = getbitu(buffer + index, 14, 10);

    index += length + 6;
  }
  return res;
}

#endif