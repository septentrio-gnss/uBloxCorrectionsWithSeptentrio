#!/bin/bash

duration=$1
wait_time=$2
#repetitions=$3

# Test sequence
sequence=("MQTT" "LBAND" "MQTT" "LBAND" "MQTT" "MQTT" "LBAND" "LBAND")

mqtt_count=0
lb_count=0

# Total Test Repetitions
for mode in "${sequence[@]}"; do

  if [ "$3" = "reset_true" ]; then
    cmd="python3 ../test/reseter.py --main_comm USB --main_config /dev/ttyACM0@115200"

    # Show and execute the command, then wait until its finishes
    echo "$cmd"
    echo -e "\n"
    eval "$cmd"
    wait
    sleep $wait_time

  fi

  if [ "$mode" == "MQTT" ]; then

    mqtt_count=$((mqtt_count+1))

    cmd="./build/gluecode \
    --mode Ip \
    --main_comm USB --main_config /dev/ttyACM0@115200 \
    --client_id 95b99023-dcc5-4710-8eef-5cfbdd362c36 \
    --reset_default true \
    --timer $duration \
    --SBF_Logging short_mqtt_$mqtt_count \
    --SBF_Logging_Config Support@sec1"

  elif [ "$mode" == "LBAND" ]; then

    lb_count=$((lb_count+1))

    cmd="./build/gluecode \
    --mode Lb \
    --main_comm USB \
    --main_config /dev/ttyACM0@115200 \
    --lband_comm USB \
    --lband_config /dev/ttyACM1@115200 \
    --client_id 95b99023-dcc5-4710-8eef-5cfbdd362c36 \
    --reset_default true \
    --timer $duration \
    --SBF_Logging short_lb_$lb_count \
    --SBF_Logging_Config Support@sec1"

  fi

  # Show the executed command
  echo "$cmd"
  echo -e "\n"

  # Execute the command
  eval "$cmd"

  # Wait for the command to finish
  wait
  
  # Sleep
  sleep "$wait_time"

done

echo "#------------------------------------#\n"
echo "#   Testing Finished                 #\n"
echo "#   Starting SPARTN Parser           #\n"
echo "#------------------------------------#\n"