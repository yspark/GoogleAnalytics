from socket import socket, AF_INET, SOCK_STREAM
from select import select
import sys
import json

from Mysql import SADS_Mysql
from Utilities import Utilities

class SADS:
    def __init__(self):
        # SADS dictionary data structure
        self.sadsDictionary = {}
        
        # MySQL object
        self.mysql = SADS_Mysql()
        self.mysql.connect()
    #end def __init__(self):
    
    # Main handler method deals with Visitor's information
    def handleVisitorInfo(self, visitorInfo):
        
        location_ip = Utilities.convertNumericIpToHex(visitorInfo['ip'])
            
        newVisitCount = self.mysql.getVisitCount(location_ip)
        
        if self.sadsDictionary.has_key(visitorInfo['ip']):
            currentVisitCount = self.sadsDictionary[visitorInfo['ip']]         
        
            if currentVisitCount != newVisitCount -1 and currentVisitCount != newVisitCount:
                print 'Warning: visit count is jumped from '+ str(currentVisitCount) + " to " + str(newVisitCount)
                
        self.sadsDictionary[visitorInfo['ip']] = newVisitCount
        
        print self.sadsDictionary
    #end def recvVisitorInfo()
#end class SADS:

        

if __name__ == "__main__":
    sads = SADS()
        
    # Listen to socket
    port = 5500
    host = ''
    backlog = 30
    
    serverSock = socket(AF_INET, SOCK_STREAM)
    serverSock.bind((host,port))
    serverSock.listen(backlog)
    serverSock.setblocking(True)
    
    clientSockList = [serverSock]
    
    while 1:
        (inputReady, outputReady, excaptReady) = select(clientSockList, [], [])
        
        for sock in inputReady:
            if sock == serverSock:
                clientSocket, address = serverSock.accept()
                clientSocket.setblocking(True)
                clientSockList.append(clientSocket)
            
                print("Packet received from: ", address)
            else:
                recv_msg = sock.recv(1500)
                
                if len(recv_msg) == 0:
                    clientSockList.remove(sock)
                    
                    # for testing
                    #serverSock.close()
                    #sys.exit()
                else:                                    
                    print("Received message: ", recv_msg, len(recv_msg))
                    (clientIp, clientPort) = sock.getpeername()
                                        
                    # add visitor's IP address
                    visitorInfo = json.loads(recv_msg)
                    visitorInfo["ip"] = clientIp   
                    
                    sads.handleVisitorInfo(visitorInfo)                                                                                     
                #end if
        #end for
    #end while
            
        
    
    
    
    
