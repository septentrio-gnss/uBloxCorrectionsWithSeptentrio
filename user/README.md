<div align="center">
  
# How to guide for USER implementation of uBlox's PointPerfect Library for GNSS Corrections

## Authors
  
| Name | GitHub |
|------|--------|
| Iker Uranga | <a href="https://github.com/IkerUranga10">IkerUranga10</a> </br> |    
| Duck Jiang | <a href="https://github.com/duckjiang">duckjiang</a> </br> |  
| Luc Gousset | <a href="https://github.com/Luc-Gousset">Luc-Gousset</a> </br> |  
   
## Septentrio Links for Users  
  
| Support                                                                          | Contact                                                                          | Septentrio Home Page                                                        |
|----------------------------------------------------------------------------------|----------------------------------------------------------------------------------|-----------------------------------------------------------------------------|
| <a href="https://web.septentrio.com/GH-SSN-support ">Septentrio Support Page</a> | <a href="https://web.septentrio.com/GH-SSN-contact ">Septentrio Contact Page</a> | <a href="https://web.septentrio.com/UBL-SSN-home">Septentrio Home Page</a> |

This guide is part of a collection of documentation on integration and implementation of third party correction services with Septentrio receivers for high accuracy positioning. This collection of documentation is intended to provide our customers with **practical examples of how they can integrate the correction services they use or want to use** with their positioning system containing a SepteSeptentriontry module. Also, all these guides are based on implementing them in the same setup example that you can see in <a href="https://github.com/septentrio-gnss/Septentrio_AgnosticCorrectionsProgram/tree/main/Receiver%20and%20Raspberry%20Setup#set-up-guide-to-use-third-parties-corrections-with-septentrios-receiver-for-precise-positioning">this section</a>.

| <a href="https://github.com/septentrio-gnss/Septentrio_AgnosticCorrectionsProgram#set-up-guide-to-use-third-party-osr-and-ssr-correction-services-with-septentrios-receivers-for-precise-positioning">To access the main GitHub page for this collection of documentation click on this link</a> |
|---|

| <a href="https://web.septentrio.com/GH-SSN-modules ">To visit the page where we offer our different GNSS modules, click here</a> |
|---|
  
## DISCLAIMER
  
As discussed above, this set of guidelines are a practical example to help Septentrio Module users and developers to integrate third party fixes. The guidelines are based on a concrete setup, which you may or may not use to follow the guidelines.

We would like you to mention our disclaimer about that setup and the guides in general before starting reading this guide.
  
| <a href="https://github.com/septentrio-gnss/Septentrio_AgnosticCorrectionsProgram/tree/main/Receiver%20and%20Raspberry%20Setup#disclaimer">Click here to know more about or Setup and and general implementation documentation disclaimer</a> |
|---|
   
</div>

## License: 

See [LICENCE](../LICENSE)

## TABLE OF CONTENTS

<!--ts-->

* [Introduction](#introduction)
* [Different operating modes](#introduction)
* [Main parts of the code](#main-parts-of-the-code)
* [How to run the demonstrator](#How-to-run-the-demonstrator)
  * [Step 1: Install code dependencies](#step-1-install-code-dependencies)
  * [Step 2: Download the source code and add the PointPerfect library](#step-2-download-the-source-code-and-add-the-pointperfect-library)
  * [Step 3: Compile the source code](#step-3-compile-the-source-code)
  * [Step 4: Add the files for MQTT authentification](#step-4-add-the-files-for-mqtt-authentification)
  * [Step 5: Run the demonstrator](#step-5-run-the-demonstrator)
* [List of parameters](#list-of-parameters)
  * [General program logic parameter list](#general-program-logic-parameter-list)
  * [Logging Configuration parameter list](#logging-configuration-parameter-list)
  * [Serial Communication parameter list](#serial-communication-parameter-list)
  * [MQTT Configuration parameter list](#mqtt-configuration-parameter-list )
* [Further information](#further-information)
* [Suggestions and improvements](#suggestions-and-improvements)
  
<!--te-->

## INTRODUCTION

This documentation is a user implementation guide for integrating PointPerfect correction services into an example system setup based on Mosaic-Go + Raspberry Pi 4. The guide to set up this setup is documented in the link below. It is a sample guide so that you can implement your own based on this system or other similar systems should you wish to do so. On Septentrio's side only tests have been done with this setup.

<div align="center">

| <a href="https://github.com/septentrio-gnss/uBloxCorrectionsWithSeptentrio#receiver-and-raspberry-pi-4-setup">Click here to visit Mosaic-Go + Raspberry Pi 4 setup preparation</a> |
|---|

</div>

This way any user who has such a setup ready can start using and testing the corrections.

However, the ultimate goal of the guide is to provide an example of integration to a similar system that works with a septentrial receiver, in this case Mosaic-Go, and with an external CPU, in this case the Raspberry Pi 4.

## DIFFERENT OPERATING MODES

The different operating modes vary depending on the SPARTN data source. As discussed in the <a href="https://github.com/septentrio-gnss/uBloxCorrectionsWithSeptentrio/blob/master/README.md#point-perfect-library">PointPerfect Library</a> section, there are two possible sources of SPARTN data, either through an <a href="https://github.com/septentrio-gnss/uBloxCorrectionsWithSeptentrio/blob/master/README.md#mqtt">MQTT</a> broker or through the beam of an LBand satellite that broadcasts this data.

Therefore, the **selection of the SPARTN data source conditions some elements of the system**, such as which topics to subscribe to for MQTT communication or whether a continuous Internet connection is required or not.

Thus, from now on we will refer to two modes of operation, these are **LBand Mode** and **MQTT Mode**. The selection and configuration of these operating modes is done through the different execution parameters of the compiled code, as described in the <a href="https://github.com/septentrio-gnss/uBloxCorrectionsWithSeptentrio/blob/master/user/README.md#list-of-parameters">parameter list section</a>.

# How to run the demonstrator
  
## Step 1: Install code dependencies

The code has some dependencies that are third-party libraries beyond the standard libraries. These third-party libraries, except for the PointPerfect Library, are Open-Source with MIT license. These are the steps for installing all the dependencies:

1- Update the package index:
```
sudo apt-get update
```

2- Install g++-10 deb package:
```
sudo apt-get install g++-10
```

3- Install nlohmann json library for Debian 11
```
sudo apt install nlohmann-json3-dev 
```

4- Install mosquitto library for Debian 11
```
sudo apt-get install libmosquitto-dev -y
```

5- Install BOOST Libraries
```
sudo apt-get install libboost-all-dev
```

6- Install CMake
```
sudo apt-get install make cmake 
```

7- Update the package index:
```
sudo apt-get update
```

Additionally, PointPerfect library is also a dependency. To obtain the PointPerfect library please consult uBlox.

## Step 2: Download the source code and add the PointPerfect library
  
To download the code, simply clone this repository, since the cluecode is located inside it, specifically in the folder called 'gluecode'. To clone the repository, enter the following command in the terminal:
  
```
git clone https://github.com/septentrio-gnss/uBloxCorrectionsWithSeptentrio.git
``` 
  
Now, copy the *inc* and *lib* folder from the PointPerfect library to the folder *ssnppl_demonstrator/ppl*

## Step 3: Compile the source code

To compile the code, just navigate to the ssnppl_demonstrator folder and execute the following command:

```
cmake .
make
```

Afterward, the *ssnppl_demonstrator* binary is generated.

## Step 4: Add the files for MQTT authentification 

In order to use the mqtt with the PointPerfect library, you need 3 files that you can download from the Thingstream platform.

* AmazonRootCA1.pem
* device-XXXXXXX-pp-cert.crt
* device-XXXXXXX-pp-key.pem

XXXXXXX refere to your client ID.

Download and copy those files onto *ssnppl_demonstrator/auth* 

## Step 5: Run the demonstrator

These are the basic command executions, without using all the available parameters, see the section below to know more about the [program's parameters](#list-of-parameters).

Navigate to src folder and run:

RUN WITH MQTT - (With Basic options)
```
./ssnppl_demonstrator/ssnppl_demonstrator --mode Ip \
--main_comm USB --main_config /dev/ttyACM0@115200 \
--client_id <your_client_ID_here> 
```

RUN WITH LBAND - (With Basic options)
```
./ssnppl_demonstrator/ssnppl_demonstrator --mode Lb --main_comm USB --main_config /dev/ttyACM0@115200 \
--lband_comm USB --lband_config /dev/ttyACM1@115200 \
--client_id <your_client_ID_here>
```


## LIST OF PARAMETERS

Below are several tables with all the information about the gluecode execution arguments. These are divided into 4 sections, each with a different theme.

<div align="center">

### General program logic parameter list

| **Name / Label** |                  **Definition**                 |  **Default Values**  |                     **Possible Values**                    |                 **Example**                | **Required** |
|:----------------:|:-----------------------------------------------:|:--------------------:|:----------------------------------------------------------:|:------------------------------------------:|:------------:|
|       mode       |         Sets the general program logic.         | **No default value** |                       Ip, Lb or Dual                       |             --mode Ip --mode Lb            |    **YES**   |
|       echo       |  Option to enable/disable messages in terminal  |       **true**       |                        true or false                       |          --echo true --echo false          |    **NO**    |
|   reset_default  | If send_cmds enabled, sends copy default config |       **true**       |                        true or false                       | --reset_default true --reset_default false |    **NO**    |
|     send_cmds    |   If enabled, sends the minimal needed config.  |       **true**       |                        true or false                       |     --send_cmds true --send_cmds false     |    **NO**    |
|       timer      |             Enables timer in seconds            |         **0**        | 0 => Timer disabled  More than 0 => That number of seconds |                 --timer 120                |    **NO**    |
  
</div>

These are of general purpose, and serve to establish the behavior of the program at the level of functionality and operation.

### Logging Configuration parameter list

<div align="center">

|   **Name / Label**  |                       **Definition**                      | **Default Values** |     **Possible Values**    |             **Example**            | **Required** |
|:-------------------:|:---------------------------------------------------------:|:------------------:|:--------------------------:|:----------------------------------:|:------------:|
|    SPARTN_logging   | Enable and name SPARTN Log file                           |      **none**      |    File Name by the user   |    --SPARTN_Logging sptartn_test   |    **NO**    |
|     SBF_Logging     | Enable SBF Logging and give a name to the file            |      **none**      |    File Name by the user   |       --SBF_Logging van_test       |    **NO**    |
|  SBF_Logging_config | If SBF_Logging enabled, select sbf stream and interval    |      **none**      | [select stream]@[interval] |  --SBF_Logging_config Support@sec1 |    **NO**    |
|     NMEA_Logging    | Enable NMEA Logging  and give a name to the file          |      **none**      |    File Name by the user   |       --NMEA_Logging van_test      |    **NO**    |
| NMEA_Logging_config | If NMEA_Logging enabled, select NMEA stream and interval. |      **none**      | [NMEA Messages]@[interval] | --NMEA_Logging_config GGA+ZDA@sec1 |    **NO**    |
  
</div>
 
These parameters define whether SPARTN data logging is to be performed from the SPARTN data source (MQTT or LBand) or from the receiver status information via NMEA or SBF (Septentrio Binary Format) message types.
  
### Serial communication parameter list

<div align="center">

|  **Name / Label** |   **Definition**   |  **Default Values**  | **Possible Values** |               **Example**              | **Required** |
|:-----------------:|:------------------:|:--------------------:|:-------------------:|:--------------------------------------:|:------------:|
|     main_comm     | Select between USB | **No default value** |         USB         |             --main_comm USB            |    **YES**   |
|  main_comm_config | Configure for USB  | **No default value** |  [port]@[baudrate]  | --main_comm_config /dev/ttyACM0@115200 |    **YES**   |
|     lband_comm    | Select between USB |       **none**       |         USB         |            --lband_comm USB            |    **NO**    |
| lband_comm_config | Configure for USB  |       **none**       |   [address]@[port]  | --main_comm_config /dev/ttyACM1@115200 |    **NO**    |
  
</div>

These parameters are used to configure the serial communication with the receiver. The main channel is mandatory but the LBand channel is required only if the selected operating mode is LBand Mode.
  
### MQTT Configuration parameter list 

<div align="center">

| **Name / Label** |     **Definition**     |       Default Values       |                **Possible Values**               |           **Example**          | **Required** |
|:----------------:|:----------------------:|:--------------------------:|:------------------------------------------------:|:------------------------------:|:------------:|
|     client_id    | Set the MQTT Client ID |    **No default value**    |                Any valid client id               | --client_id [client id number] |    **YES**   |
|    mqtt_server   | Set the MQTT Server    | **pp.services.u-blox.com** |               Server never changes               | --mqtt_server [server address] |    **NO**    |
|      region      | Set the MQTT Region    |           **eu**           | UBlox coverage available regions (see their web) |           --region eu          |    **NO**    |

</div>

These parameters are used to configure the MQTT client. Normally only the client ID, obtained from the thingstream platform, is required.
    



## FURTHER INFORMATION

If you want to know more details about how the code works, you can visit the documentation for developers:
(Part of the information provided in this guide [user documentation] is repeated in developers documentation.)


<div align="center">

| <a href="https://github.com/septentrio-gnss/uBloxCorrectionsWithSeptentrio/tree/master/dev#how-to-guide-for-developer-implementation-of-ubloxs-pointperfec-library-for-gnss-corrections">Click here to visit the documentation for developers</a> |
|---|
   
</div>

## SUGGESTIONS AND IMPROVEMENTS
  
There are several thing that could be improved in the code for a better performance, stability or to be more user friendly. These things are listed here. If you want to contribute or you have some feedback or suggestion do not hestitate to share it with us!
  
<div align="center">

| <a href="https://github.com/septentrio-gnss/uBloxCorrectionsWithSeptentrio#suggestions-for-improvements">Click here to navigate to Suggestions and Improvements section</a> |
|---|
   
</div>
