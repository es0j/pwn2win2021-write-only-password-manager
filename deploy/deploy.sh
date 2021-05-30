#!/bin/bash

#this deploy doesent use docker, it need to run on bare metal
target=34.69.232.66
scp ./server $target:/tmp/server
scp ./daemon.py $target:/tmp/daemon.py

#upload important files 
ssh $target "sudo apt update; \
        sudo apt install -y python3 cpuid ;\
        sudo mv /tmp/server /root/server ;\
        sudo mv /tmp/daemon.py /root/daemon.py ;\
        sudo chmod 500 /root/daemon.py /root/server ;\
        sudo useradd almostnobody;"

#make sure no dup processes are running 
ssh  $target "sudo sh -c 'kill -9 \$(pidof /usr/bin/python3 ./daemon.py)' "
ssh  $target "sudo sh -c 'kill -9 \$(pidof server)'"

#clear /tmp just to be sure
ssh  $target "sudo sh -c 'rm -rf /tmp/*"

#run server
ssh -n -f $target "sudo sh -c 'sudo su; cd /root; nohup ./server > server.out 2>&1 &'"

#run deammon
ssh -n -f $target "sudo sh -c 'sudo su; cd /root; \
                    PYTHONUNBUFFERED=1 \
                    HOST=0.0.0.0 \
                    PORT=1337 \
                    NUSERS=800 \
                    TIMEOUT=10 \
                    TIME_TO_SLEEP=30 \
                    INTERVAL_PER_IP=120 \
                    MAX_FILE_SIZE=30000 \
                    BLOCKED_IP=0 \
                    nohup ./daemon.py > daemon.out  2>&1 &'"

#check if its ok
ssh $target 'ps -aux'
