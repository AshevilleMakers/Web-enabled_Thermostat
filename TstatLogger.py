'''
This code receives log messages from the thermostats and saves them to a log file
'''

import socket
import datetime
import pymysql
import sys
import time

###THESE VALUES MUST BE UPDATED###
host, port = '', 1700  #Change this to another port if port 1800 already in use on your Rasperry Pi
PASSWORD = '4321'   #Enter a 4 digit number here and aave it - this is your communication password and must match the web pages and arduino sketch
HeavyLog = False  #True means will write much more data to the log file
user_id = 1234 #Put the number you used as your user id in the thermostats table here - e.g. 1234
MySQL_User = 'root'
MySQL_Pwd = 'ashwood'
database_name = 'tstat_database'
logFilePrefix = '/home/pi/Tstat_Data/Tstat_Log_'
##################################

backlog = 5


def SimpleHandler(mySock):
    '''Code to handle a connection from a node'''
    totalData = ''
    Done = False
    global HeavyLog
    global logFile

    print "In SimpleHandler"
    if (HeavyLog): logFile = open(logFilePrefix + datetime.datetime.now().strftime("%y%m%d") + ".txt","a")
    data = ''
    startTime = datetime.datetime.now()
    while not Done:
        try:
            data += mySock.recv(256) #stripping whitespace caused an error when the date of a datetime is sent separate from the time of the same datetime since we need that space between them
        except socket.error as (errnum,msg):
            if mySock:
                mySock.close()
            if (not HeavyLog): logFile = open(logFilePrefix + datetime.datetime.now().strftime("%y%m%d") + ".txt","a")
            logFile.write("Socket error while trying to read data: " + msg + "\n")
            if (not HeavyLog): logFile.close();
            return
        if '#' in data:
            totalData += data[:data.find('#')]
            if (HeavyLog): logFile.write("Bytes received: " + str(len(totalData)) +"\n")
            ParsedMessage = totalData.split('/')
#            print ParsedMessage[0]
#            print ParsedMessage[1]
#            print ParsedMessage[2]
#            print ParsedMessage[3]
            if (ParsedMessage[1] != PASSWORD):
                if (not HeavyLog): logFile = open(logFilePrefix + datetime.datetime.now().strftime("%y%m%d") + ".txt","a")
                logFile.write("Bad password: " + totalData + "\n")
                logFile.write("Time:  " + datetime.datetime.now().strftime("%m/%d/%y %I:%M:%S %p") + "\n")
                logFile.write("_____________________\n")
                if (not HeavyLog): logFile.close();
                mySock.sendall("&" + ParsedMessage[0][1:] + "/" + ParsedMessage[1] + "/" + "0/" + "0\n")
                mySock.close()
                return

            if (ParsedMessage[0] == "!MSG"):  #This is a message for the log file
                if (not HeavyLog): logFile = open(logFilePrefix + datetime.datetime.now().strftime("%y%m%d") + ".txt","a")
                logFile.write("Sender:  " + ParsedMessage[2] + "\n")
                logFile.write("Time:  " + datetime.datetime.now().strftime("%m/%d/%y %I:%M:%S %p") + "\n")
                logFile.write("Message: " + ParsedMessage[3] + "\n")
                logFile.write("_____________________\n")
                if (not HeavyLog): logFile.close();
                mySock.sendall("&" + ParsedMessage[0][1:] + "/" + ParsedMessage[1] + "/" + "0/" + "1\n")
            else:  #This is a bad message type
                if (not HeavyLog): logFile = open("/home/pi/LC_Data/LC_Log_" + TestFile + datetime.datetime.now().strftime("%y%m%d") + ".txt","a")
                logFile.write("Bad message type: " + totalData + "\n")
                logFile.write("Time:  " + datetime.datetime.now().strftime("%m/%d/%y %I:%M:%S %p") + "\n")
                logFile.write("_____________________\n")
                if (not HeavyLog): logFile.close();
                mySock.sendall("&" + ParsedMessage[0][1:] + "/" + ParsedMessage[1] + "/" + "0/" + "0\n")
    
            Done = True
            mySock.close()
            if (HeavyLog): logFile.close()
            print "Received: " + totalData
    
        if (datetime.datetime.now() - startTime)> datetime.timedelta(seconds=3):
            Done=True
            mySock.close()
            if (not HeavyLog): logFile = open(logFilePrefix + datetime.datetime.now().strftime("%y%m%d") + ".txt","a")
            logFile.write("Timeout while trying to read data: " + totalData + "\n")
            if (not HeavyLog): logFile.close();
            print "Timeout while trying to read data: " + totalData
            return

           


#This is the start of the Main code
#Save message indicating restart to log file        

logFile = open(logFilePrefix + datetime.datetime.now().strftime("%y%m%d") + ".txt","a")
logFile.write("\nTstatLogger server restart: " + datetime.datetime.now().strftime("%m/%d/%y %I:%M:%S") + "\n\n")
logFile.write("_____________________\n")
logFile.close()

#start listening forever
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.bind((host,port))
s.listen(backlog)
print "running " + sys.argv[0][sys.argv[0].rfind('/')+1:]
print "serving at port ", port
while 1:
    client, address = s.accept()
    print "----------------------------------"
    print datetime.datetime.now()
    print "Accepted client."
    SimpleHandler(client)
    print "Back from handler."
    print datetime.datetime.now()
    
