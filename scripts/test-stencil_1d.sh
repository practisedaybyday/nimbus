#!/usr/bin/env bash

#
#  Copyright 2013 Stanford University.
#  All rights reserved.
# 
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
# 
#  - Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 
#  - Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the
#    distribution.
# 
#  - Neither the name of the copyright holders nor the names of
#    its contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
# 
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
#  FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
#  THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
#  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
#  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
#  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
#  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
#  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
#  OF THE POSSIBILITY OF SUCH DAMAGE.
#

# Starts Nimbus controller on the machine this script is executed on.

# Author: Omid Mashayekhi <omidm@stanford.edu>

# **************************
# Text Reset
RCol='\x1B[0m'    
# Regular           
Bla='\x1B[0;90m';
Red='\x1B[0;91m';
Gre='\x1B[0;92m';
Yel='\x1B[0;93m';
Blu='\x1B[0;94m';
Pur='\x1B[0;95m';
Cya='\x1B[0;96m';
Whi='\x1B[0;97m';
# **************************

function print_usage {
  echo -e "${Cya}Runs comprehenvive tests for controller/worker"
  echo -e "${Cya}against stencil_1d application, tests include: "
  echo -e "   1. XXXX:"
  echo -e "   2. YYYY:"
  echo -e "\nUsage:"
  echo -e "./scripts/test-stencil_1d.sh"
  echo -e "${RCol}"
}


TIME_OUT_T=10

if [ -z "${NIMBUS_HOME}" ]; then
  export NIMBUS_HOME="$(cd "`dirname "$0"`"/..; pwd)"
fi

if [ -z "${DBG}" ]; then
  export DBG="error"
fi

if [ -z "${TTIMER}" ]; then
  export TTIMER="l1"
fi


# run_experiment controller_args worker_args app_args
function run_experiment {
  ${NIMBUS_HOME}/scripts/stop-workers.sh &> /dev/null
  ${NIMBUS_HOME}/scripts/stop-controller.sh &> /dev/null
  ${NIMBUS_HOME}/scripts/start-controller.sh $1 &> /dev/null
  sleep 2
  ${NIMBUS_HOME}/scripts/start-workers.sh $2 -l applications/simple/stencil_1d/libstencil_1d.so $3 &> /dev/null
  
  start_time=$(date +%s)
  end_time=$(date +%s)
  progress_bar="waiting ..."
  success=0
  while [ "$((${end_time}-${start_time}))" -lt "${TIME_OUT_T}" ]; do
    success=$(cat ${NIMBUS_HOME}/logs/workers/*/stdout | grep -c "FINAL HASH")
    if [ ${success} == "1" ]; then
      end_time=$(date +%s)
      break
    else
      end_time=$(date +%s)
      echo -ne "${Yel}${progress_bar} \r${RCol}"
      progress_bar=${progress_bar}"."
      sleep 1
    fi
  done
  
  if [ ${success} == "1" ]; then
    echo -e "${Gre}[ SUCCESS ] finished in $((${end_time}-${start_time})) seconds.${RCol}"
  else
    echo -e "${Red}[ TIMEOUT ] experiment did not finish before time out!${RCol}"
    exit 1
  fi
}

function get_final_hash {
  local final_hash=$(grep "FINAL HASH" logs/workers/*/stdout  | sed 's/.*FINAL HASH: //')
  echo "${final_hash}"
}

# check_hash correct_hash
function check_hash {
  local final_hash=$(grep "FINAL HASH" logs/workers/*/stdout  | sed 's/.*FINAL HASH: //')
  if [ "${final_hash}" == "$1" ]; then
    echo -e "${Gre}[ SUCCESS ] hash value matches!${RCol}"
  else
    echo -e "${Red}[ FAILED  ] hash value does not match! [${final_hash} != $1]${RCol}"
    exit 1
  fi
}


if [[ "$1" == "-h" ]] || [[ "$1" == "--help" ]]; then
  print_usage
  exit 0
fi

echo -e "${Cya}Ruunnig experiment for:${RCol}"
echo -e "${Cya}    a. single-threaded controller${RCol}"
echo -e "${Cya}    b. one single-threaded worker${RCol}"
echo -e "${Cya}    c. one application partition${RCol}"
echo -e "${Cya}    d. without any templates${RCol}"
run_experiment "--dct" "1" "--pn 1 --cpp 16" 
correct_hash=$(get_final_hash)


echo -e "${Cya}Ruunnig experiment for:${RCol}"
echo -e "${Cya}    a. single-threaded controller${RCol}"
echo -e "${Cya}    b. one single-threaded worker${RCol}"
echo -e "${Cya}    c. 16 application partitions${RCol}"
echo -e "${Cya}    d. without any templates${RCol}"
run_experiment "--dct" "1" "--pn 16 --cpp 1" 
check_hash "${correct_hash}"


echo -e "${Cya}Ruunnig experiment for:${RCol}"
echo -e "${Cya}    a. multi-threaded controller${RCol}"
echo -e "${Cya}    b. one single-threaded worker${RCol}"
echo -e "${Cya}    c. 16 application partitions${RCol}"
echo -e "${Cya}    d. without any templates${RCol}"
run_experiment "-t 8 -a 6 --dct" "1" "--pn 16 --cpp 1" 
check_hash "${correct_hash}"


echo -e "${Cya}Ruunnig experiment for:${RCol}"
echo -e "${Cya}    a. multi-threaded controller${RCol}"
echo -e "${Cya}    b. one single-threaded worker${RCol}"
echo -e "${Cya}    c. 16 application partitions${RCol}"
echo -e "${Cya}    d. with only controller templates${RCol}"
run_experiment "-t 8 -a 6 --dbm" "1" "--pn 16 --cpp 1" 
check_hash "${correct_hash}"


echo -e "${Cya}Ruunnig experiment for:${RCol}"
echo -e "${Cya}    a. multi-threaded controller${RCol}"
echo -e "${Cya}    b. one single-threaded worker${RCol}"
echo -e "${Cya}    c. 16 application partitions${RCol}"
echo -e "${Cya}    d. with controller and worker templates${RCol}"
run_experiment "-t 8 -a 6" "1 --det " "--pn 16 --cpp 1" 
check_hash "${correct_hash}"


echo -e "${Cya}Ruunnig experiment for:${RCol}"
echo -e "${Cya}    a. multi-threaded controller${RCol}"
echo -e "${Cya}    b. one single-threaded worker${RCol}"
echo -e "${Cya}    c. 16 application partitions${RCol}"
echo -e "${Cya}    d. with controller, worker, and execution templates${RCol}"
run_experiment "-t 8 -a 6" "1" "--pn 16 --cpp 1" 
check_hash "${correct_hash}"


echo -e "${Cya}Ruunnig experiment for:${RCol}"
echo -e "${Cya}    a. multi-threaded controller${RCol}"
echo -e "${Cya}    b. two single-threaded worker${RCol}"
echo -e "${Cya}    c. 16 application partitions${RCol}"
echo -e "${Cya}    d. with controller, worker, and execution templates${RCol}"
run_experiment "-t 8 -a 6 -w 2 --split 2 1 1" "2" "--pn 16 --cpp 1" 
check_hash "${correct_hash}"


echo -e "${Cya}Ruunnig experiment for:${RCol}"
echo -e "${Cya}    a. multi-threaded controller${RCol}"
echo -e "${Cya}    b. four single-threaded worker${RCol}"
echo -e "${Cya}    c. 16 application partitions${RCol}"
echo -e "${Cya}    d. with controller, worker, and execution templates${RCol}"
run_experiment "-t 8 -a 6 -w 4 --split 4 1 1" "4" "--pn 16 --cpp 1" 
check_hash "${correct_hash}"


echo -e "${Cya}Ruunnig experiment for:${RCol}"
echo -e "${Cya}    a. multi-threaded controller${RCol}"
echo -e "${Cya}    b. one multi-threaded worker${RCol}"
echo -e "${Cya}    c. 16 application partitions${RCol}"
echo -e "${Cya}    d. with controller, worker, and execution templates${RCol}"
run_experiment "-t 8 -a 6" "1 --othread 16" "--pn 16 --cpp 1" 
check_hash "${correct_hash}"


echo -e "${Cya}Ruunnig experiment for:${RCol}"
echo -e "${Cya}    a. multi-threaded controller${RCol}"
echo -e "${Cya}    b. four multi-threaded worker${RCol}"
echo -e "${Cya}    c. 16 application partitions${RCol}"
echo -e "${Cya}    d. with controller, worker, and execution templates${RCol}"
run_experiment "-t 8 -a 6 -w 4 --split 4 1 1" "4 --othread 16" "--pn 16 --cpp 1" 
check_hash "${correct_hash}"


echo -e "${Gre}\n[ PASSED  ] all tests passed successfuly!${RCol}"
exit 0


