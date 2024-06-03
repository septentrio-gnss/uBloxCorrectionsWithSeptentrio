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

#ifndef __UTILS__
#define __UTILS__

#include <vector>
#include <iostream>
#include <string>
#include <cstdint>
#include <cmath>

constexpr float EARTHRADIUS{6371.0};
constexpr float radians(const float degree){return (degree *(M_PI/180));} ; 


void echo(const std::string &msg, bool echo_mode);

std::vector<std::string> split(const std::string &str, char separator);

bool is_empty(const uint8_t *arr, std::size_t size);

unsigned int getbitu(const unsigned char *buff, int pos, int len);

std::vector<int> identifyRTCM3MessageIDs(const uint8_t *buffer, size_t bufferSize);

float distance_between_locations(const float lat1 , const float lon1 ,const float lat2 , const float lon2);

float NMEAToDecimal(const std::string& Coord , const std::string& Direction) noexcept;

#endif