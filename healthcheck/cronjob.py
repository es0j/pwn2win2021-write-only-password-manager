#!/usr/bin/python3
import subprocess

def runcmd(cmd):
    return subprocess.Popen("ssh 34.69.232.66 {cmd}".format(cmd=cmd), shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()





upcmd="""sudo sh -c 'sudo su; cd /root; nohup ./server > server.out 2>&1 &'"""
checkcmd="""sudo ps -aux | grep ./server"""


print(runcmd(checkcmd))

