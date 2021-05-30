#!/usr/bin/python3
import threading
import traceback
import socket
import time
import collections
import os
import subprocess


HOST=os.getenv("HOST")
PORT=int(os.getenv("PORT"))
NUSERS=int(os.getenv("NUSERS"))
TIMEOUT=int(os.getenv("TIMEOUT"))
TIME_TO_SLEEP=float(os.getenv("TIME_TO_SLEEP"))
INTERVAL_PER_IP=float(os.getenv("INTERVAL_PER_IP"))
BLOCKED_IP=os.getenv("BLOCKED_IP").split(",")
MAX_FILE_SIZE=int(os.getenv("MAX_FILE_SIZE"))

#important for the challenge, check it on server
os.system("head /proc/cpuinfo")

print(f"""[INFO] Starting server in {HOST}:{PORT}
[INFO] NUSERS={NUSERS}
[INFO] TIMEOUT={TIMEOUT}
[INFO] TIME_TO_SLEEP={TIME_TO_SLEEP}
[INFO] INTERVAL_PER_IP={INTERVAL_PER_IP}
[INFO] BLOCKED_IP={BLOCKED_IP}
[INFO] MAX_FILE_SIZE={MAX_FILE_SIZE}
""")



class connList(list):
    def pop(self, *args, **kwargs):
        ret=list.pop(self,*args,**kwargs)
        try:
            self.showQueue()
        except:
            pass
        return ret 

    def showQueue(self):
        for i in range(len(self)):
            self[i]["socket"].sendall(b"Position in queue: %i\n"%(i+1))
    

def recvall(sock, n):
    data = b""
    while len(data) < n:
        packet = sock.recv(n - len(data))
        if not packet:
            return b""
        data+=packet
    return data




class server:
    def __init__(self):
        
        self.todo=connList()
        self.antispam={}

        self.sem = threading.Semaphore()
        threading.Thread(target=self.runner_thread).start()

        self.server=socket.socket(socket.AF_INET,socket.SOCK_STREAM)
        self.server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.server.bind((HOST,PORT))
        self.server.listen(NUSERS)

        self.mainloop()

    def mainloop(self):
        while 1:
            try:
                conn,addr = self.server.accept()
                print("[INFO] Connected with %s. Todo length: %i"%(addr,len(self.todo)))
                threading.Thread(target=self.warpWelcome,args=(conn,addr)).start()
            except Exception: 
                print("[ERROR] unkown Exception occurred")
                traceback.print_exc()

    def warpWelcome(self,conn,addr):
        try:
            self.welcome(conn,addr)
             
        except socket.timeout:
            conn.sendall(b"Timeout reached. Bye!\n")
            conn.close()
        except IOError:
            print("[INFO] client disconected")
            

    def welcome(self,conn,addr):
        
        ip=addr[0]
        conn.settimeout(TIMEOUT)
        
        if ip in BLOCKED_IP:
            print("[INFO] BLOCKED IP tried to access: %s ."%ip)
            conn.sendall(b"Looks like you're blocked conntact an admin. Bye!\n")
            conn.close()
            return

        if (ip in self.antispam) and len(self.todo) > 0 :
            dtime=time.time()-self.antispam[ip]
            if dtime < INTERVAL_PER_IP:
                print("[INFO] Repeated request made too quickly from same IP: %s ."%ip)
                conn.sendall(b"Wow, too fast! Try again after %i seconds!\n"%(INTERVAL_PER_IP -dtime))
                conn.close()
                return

        self.antispam[ip]=time.time()


        conn.sendall(b"Hello there. Send me your ELF.\n")
        conn.sendall(b"Give me how many bytes (max: %i)\n"%MAX_FILE_SIZE)
        response=conn.recv(len(str(MAX_FILE_SIZE))+2)
        try:
            lenbytes=int(response)
        except:
            print("[INFO] Erro in size conversion.")
            conn.sendall(b"Error in size conversion. Bye!\n")
            conn.close()
            return

        if lenbytes>MAX_FILE_SIZE or lenbytes<=0:
            print("[INFO] Invalid number.")
            conn.sendall(b"invalid number. Bye!\n")
            conn.close()
            return
        else:
            conn.sendall(b"Send 'em!\n")
            payload=recvall(conn,lenbytes)

        if payload[:4]!=b"\x7fELF":
            print("[INFO] Not ELF format.")
            conn.sendall(b"Not ELF. bye!\n")
            conn.close()
            return
        else:
            self.sem.acquire()
            pos=len(self.todo)+1
            self.todo.append({"socket":conn,"payload":payload})
            self.sem.release()
            conn.sendall(b"Position in queue: %i\n"%pos)
            return
    
    def runner_thread(self):
        while True:
            try:
                if len(self.todo)>0:
                    self.sem.acquire()
                    job=self.todo.pop(0)
                    self.sem.release()

                    conn=job["socket"]
                    payload=job["payload"]
                    #conn.sendall(b"Executing your file plz wait...\n")

                    print("[INFO] Running job!")
                    conn.sendall(b"Running your task, please wait \n")
                    resp=self.execute(payload,conn)

                else:
                    time.sleep(0.2)
            except:
                print("[ERROR] RUNNER CRASHED!")
                traceback.print_exc()

    def execute(self,bytesarg,conn):

        #exclusive cpu access
        os.system("rm -rf /tmp/*")
        os.system("pkill -9 -u `id -u almostnobody`")
              
    
        with open("/tmp/executable.bin","wb") as f:
            f.write(bytesarg)


        os.system("chmod +x /tmp/executable.bin")

        start=time.time()
        executable=subprocess.Popen("su -s /bin/bash -m almostnobody -c /tmp/executable.bin",stdout=conn.fileno(),stderr=conn.fileno(),shell=True)
        try:
            executable.wait(timeout=TIME_TO_SLEEP)
        except subprocess.TimeoutExpired: 
            print("[INFO] Process timedout.")
            conn.sendall(b"Time out.\n")
        
        conn.close()
        os.system("pkill -9 -u `id -u almostnobody`")
        os.system("rm -rf /tmp/*")
        

        print("[INFO] Job executed in %f seconds"%(time.time()-start))        
        
        
                


if __name__=="__main__":
    x=server()
