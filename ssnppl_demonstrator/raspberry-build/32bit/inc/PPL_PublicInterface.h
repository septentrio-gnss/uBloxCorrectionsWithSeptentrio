/*
 * Copyright 2019-2023 u-blox AG. All rights reserved.
 *
 * PROTECTED SOURCE CODE - PROPRIETARY AND CONFIDENTIAL.
 * This file is licensed as Protected Source Code under the terms of your
 * agreement with u-blox AG. You may compile the file and incorporate the
 * resulting executable code in a device for the sole purpose of enabling
 * communication between a device and the u-blox services.
 *
 * For full details of the license terms that govern the use of this file,
 * please refer to the u-blox Service Access and Use Terms available at
 * https://portal.thingstream.io/terms.
 */

/**
  * @file
  * @brief PointPerfect SDK API
  *
  * Public interface for the PointPerfect SDK
  */

 // * * * * * * * * * * * * * * * * * * * * * * * * *
 // Released from Library Version Number 20221108.7
 // * * * * * * * * * * * * * * * * * * * * * * * * *

#ifndef PPL_PublicInterface_H
  #define PPL_PublicInterface_H


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * Enumerator for the general return statuses of the PPL interface functions
 */

typedef enum _ePPL_ReturnStatus
{
  /** The successful PointPerfect API response */
  ePPL_Success = 0,

  /** A library interface has been used incorrectly (e.g. passed in a
   * null pointer)
   */
  ePPL_IncorrectLibUsage,

  /** PPL_Initialize() was never called, or not enough heap space for
   * the library
   */
  ePPL_LibInitFailed,

  /** Library has expired */
  ePPL_LibExpired,

  /** Use of L-Band channel when not initialized */
  ePPL_LBandChannelNotEnabled,

  /** Use of IP channel when not initialized */
  ePPL_IPChannelNotEnabled,

  /** Use of auxilliary channel when not initialized */
  ePPL_AuxChannelNotEnabled,

  /** Stream is encrypted, and no dynamic key available. Check that
   * PPL_SendDynamicKey() returns success
   */
  ePPL_NoDynamicKey = 100,

  /** Push of dynamic key to library failed */
  ePPL_FailedDynKeyLibPush,

  /** Reported length of key provided is invalid or key data contains an
   * invalid (non-hex) character
   */
  ePPL_InvalidDynKey,

  /** Valid key, but doesn't match the stream. Verify that key data
   * provided is the correct key for the SPARTN stream and has not expired
   */
  ePPL_IncorrectDynKey,

  /** NMEA-GGA message not enabled for F9 output, F9 output stream not
   * being correctly passed into PPL_SendRcvrData(), or GGA data does not
   * have required Quality Indicator
   */
  ePPL_RcvPosNotAvailable = 200,

  /** Leap seconds not available for decoding of GLONASS message content.
   * This message may be transient. F9 users can enable UBX-NAV-TIMELS for
   * immediate access to the leap seconds from the receiver. Third party
   * receivers may need to wait up to 30-60 seconds after startup for GLONASS
   * corrections to be available. However, corrections for non-GLONASS
   * constellations may still be available.
   */
  ePPL_LeapSecsNotAvailable,

  /** SPARTN area definition message is delayed or outside of
   * PointPerfect coverage
   */
  ePPL_AreaDefNotAvailableForPos,

  /** Library has yet to receive a full SPARTN timetag or ZDA message to
   * resolve current time
   */
  ePPL_TimeNotResolved,

  /** Potential buffer overflow */
  ePPL_PotentialBuffOverflow = 300,

  /** Unexpected behavior, or unknown state */
  ePPL_UnknownState = 999

} ePPL_ReturnStatus;


//-----------------------------------------------------------------------------------------
/* Configuration options for the Point Perfect Library */
//-----------------------------------------------------------------------------------------
/** Dual channel: IP and L-Band support
 * @hideinitializer
 */
#define PPL_CFG_DEFAULT_CFG           0x00000000LU

/** Enable IP channel support
 * @hideinitializer
 */
#define PPL_CFG_ENABLE_IP_CHANNEL     0x00000001LU

/** Enable L-Band channel support
 * @hideinitializer
 */
#define PPL_CFG_ENABLE_LBAND_CHANNEL  0x00000002LU

/** Enable auxiliary channel support
 * @hideinitializer
 */
#define PPL_CFG_ENABLE_AUX_CHANNEL    0x00000004LU

//-----------------------------------------------------------------------------------------

/**
 * Initialize the PointPerfect library
 *
 * @param u32PPLConfigOptionsMask  bitmask specifying the configuration of the library
 *
 * @return #ePPL_ReturnStatus indicating success/failure
 *
 * @note Always call at start-up of the system. No need to call at any other time.
 * @note Auxiliary and L-band channels cannot be initialized simultaneously.
 *
 */
ePPL_ReturnStatus PPL_Initialize(const uint32_t u32PPLConfigOptionsMask);


/** Size of 128-bit key in bytes */
static const uint8_t u8DynamicKey128BitsInBytes = 16;
/** Size of 192-bit key in bytes */
static const uint8_t u8DynamicKey192BitsInBytes = 24;
/** Size of 256-bit key in bytes */
static const uint8_t u8DynamicKey256BitsInBytes = 32;
/** Maximum size of the key in bytes */
static const uint8_t u8MaxDynamicKeyLength = 32; // u8DynamicKey256BitsInBytes;

/**
 * Pushes dynamic key data into library. Only 128-bit, 192-bit, and 256-bit keys are
 * permitted.
 *
 * @param pcDynKeyHexString dynamic key hex (ASCII) char buffer
 * @param u8DynKeyHexStringSize number of chars in the buffer (do not include null-terminator!)
 *
 * @return #ePPL_ReturnStatus indicating success/failure
 *
 */
ePPL_ReturnStatus PPL_SendDynamicKey(const char   *pcDynKeyHexString,
                                     const uint8_t u8DynKeyHexStringSize);

/**
 * Send a buffer of bytes to the library from a SPARTN stream. Method pushes the
 * SPARTN message stream to the library for further processing. A method to retrieve
 * library output should be called immediately following successful return from this
 * method as output may have been generated.
 *
 * @param pvSpartnStreamBuffer SPARTN binary stream buffer
 * @param u16SpartnStreamBufferSize Size (bytes) of the SPARTN binary stream buffer
 *
 * @return #ePPL_ReturnStatus indicating success/failure
 *
 * @note This interface requires that the IP channel is configured for the library. If
 *       the channel is not configured and data is passed in the following
 *       error code will be returned: #ePPL_IPChannelNotEnabled
 *
 *
 */
ePPL_ReturnStatus PPL_SendSpartn(const void      *pvSpartnStreamBuffer,
                                 const uint16_t   u16SpartnStreamBufferSize);

/**
 * Send a buffer of bytes to the library from a UBLOX L-Band receiver that outputs
 * SPARTN stream data wrapped in a UBX-RXM-PMP receiver message. Method removes the UBX
 * message wrapper and forwards the SPARTN message stream to the library for further
 * processing. A method to retrieve library output should be called immediately
 * following successful return from this method as output may have been generated.
 *
 * @param pvUbxLbandStreamBuffer UBX binary stream buffer
 * @param u16UbxLbandStreamBufferSize Size (bytes) of the UBX L-band binary stream buffer
 *
 * @return #ePPL_ReturnStatus indicating success/failure
 *
 * @note This interface requires that the L-Band channel is configured for the library. If
 *       the channel is not configured and data is passed in the following
 *       error code will be returned #ePPL_LBandChannelNotEnabled
 * @note This channel cannot be called if the PPL_SendAuxSpartn() interface is in use. This
 *       will cause the library to output intermittent data.
 *
 *
 */
ePPL_ReturnStatus PPL_SendUBXLBand(const void      *pvUbxLbandStreamBuffer,
                                   const uint16_t   u16UbxLbandStreamBufferSize);

/**
 * Send a buffer of bytes to the library from an auxiliary SPARTN source that outputs
 * SPARTN stream data raw form (which may include fill characters between SPARTN messages
 * as is the case for some L-Band receiver sources). A method to retrieve library output
 * should be called immediately following successful return from this method as output
 * may have been generated.
 *
 * @param pvAuxSpartnStreamBuffer Auxiliary SPARTN binary stream buffer, without
 *                                additional frame wrapping (i.e, raw SPARTN, which may
 *                                include the additional fill characters).
 * @param u16AuxSpartnStreamBufferSize  size (bytes) of the auxiliary binary stream buffer
 *
 * @return #ePPL_ReturnStatus indicating success/failure
 *
 * @note This interface requires that the Auxiliary channel is configured for the
 *       library. If the channel is not configured and data is passed in the following
 *       error code will be returned #ePPL_AuxChannelNotEnabled
 * @note This channel cannot be called if the PPL_SendUBXLBand() interface is in use. This
 *       will cause the library to output intermittent data.
 *
 *
 */
ePPL_ReturnStatus PPL_SendAuxSpartn(const void     *pvAuxSpartnStreamBuffer,
                                    const uint16_t  u16AuxSpartnStreamBufferSize);

/**
 * Used to send a buffer of bytes to the library from a receiver that outputs
 * ephemeris data and position data.
 * Method decodes GNSS ephemeris data and forwards the ephemeris data to the
 * library for further processing. Position is used to compute virtual observations for
 * RTK positioning. Also supports optional time and leap second information for faster startup.
 *
 * @param pvRcvrStreamBuffer stream buffer
 * @param u16RcvrStreamBufferSize size (bytes) of the receiver message binary stream buffer
 *
 * @return #ePPL_ReturnStatus indicating success/failure
 *
 */
ePPL_ReturnStatus PPL_SendRcvrData(const void      *pvRcvrStreamBuffer,
                                   const uint16_t   u16RcvrStreamBufferSize);


/** The maximum length (in bytes) of RTCM messages
 *  @hideinitializer
 */
/*   19 is the length of RTCM 1005,
 *  164 is the maximum length of RTCM 1033,
 *   12 is the length of RTCM 1230,
 *  450 is the maximum length of each RTCM MSM4 message
 *       (up to 2 for GPS, 1 for GLONASS, up to 2 for Galileo, up to 2 for BeiDou)
 */
#define PPL_MAX_RTCM_BUFFER (19 + 164 + 12 + (450 * 7))

/**
 * Used to format GNSS Observation data from the library into RTCM format.
 * Method encodes GNSS observation data using the RTCM protocol and outputs
 * data to the user provided buffer.
 *
 * @param pvBuf pointer to buffer that will contain the RTCM message stream
 * @param u32BufMaxSize max size of the input buffer.
 *                        #PPL_MAX_RTCM_BUFFER is the suggested length
 * @param pu32CopiedToBuf pointer to variable to contain the number of bytes copied
 *                        to the buffer. 0 if encoded message data is larger than the
 *                        buffer or there has been no message created.
 *
 * @return #ePPL_ReturnStatus indicating success/failure
 *
 */
ePPL_ReturnStatus PPL_GetRTCMOutput( void          *pvBuf,
                                     const uint32_t u32BufMaxSize,
                                     uint32_t      *pu32CopiedToBuf);


#ifdef __cplusplus
}
#endif

#endif // #ifndef PPL_PublicInterface_H
