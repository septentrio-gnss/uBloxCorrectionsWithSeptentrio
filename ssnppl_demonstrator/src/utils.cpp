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



#include <vector>
#include <iostream>
#include <string>
#include <cstdint>
#include <cstring>
#include <sstream>
#include "utils.hpp"

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

float distance_between_locations( float lat1 ,  float lon1 , float lat2 ,  float lon2)
{
    lat1 = radians(lat1);
    lon1 = radians(lon1);
    lat2 = radians(lat2);
    lon2 = radians(lon2);

    float dlon = lon2 - lon1;
    float dlat = lat2 - lat1;
    float result = pow(sin(dlat / 2), 2) + 
                          cos(lat1) * cos(lat2) * 
                          pow(sin(dlon / 2), 2);
 
    result = 2 * asin(sqrt(result));

    result = result * EARTHRADIUS;

    return result;
}

float NMEAToDecimal(const std::string& Coord , const std::string& Direction) noexcept
{
    float degree = std::stof(Coord.substr(0,2));
    float minute = std::stof(Coord.substr(2));
    float decimal = degree + minute / 60.0 ;
    if (Direction =="S" || Direction == "W"){
        decimal = -decimal ;
    }
    return decimal ;
}

