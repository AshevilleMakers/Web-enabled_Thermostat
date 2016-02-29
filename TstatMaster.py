'''
This is code that controls the thermostats as a master.  It sends instructions
to the nodes and receives a response.  Typically these commands come from the web-site or some other
source, and the response is then passed back to that port that originated the instructions.  As such,
the core functionality of this routine is to receive instructions from various sources and pass them
to the correct node, then relay the response back to the originating source, as well as to keeo track
of internal representations of the thermostats'.

'''


import socket
import datetime
import pymysql
import sys
import time
import Thermostat
import select

###THESE VALUES MUST BE UPDATED###
host, serverPort = '', 1800  #Change this to another port if port 1800 already in use on your Rasperry Pi
PASSWORD = '4321' #Enter a 4 digit number here and aave it - this is your communication password
HeavyLog = False  #True means will write much more data to the log file
user_id = 1234 #Put the number you used as your user id in the thermostats table here - e.g. 1234
MySQL_User = 'root'
MySQL_Pwd = 'ashwood'
database_name = 'tstat_database'
logFilePrefix = '/home/pi/Tstat_Data/Tstat_Log_'
##################################

backlog = 5



def SimpleHandler(mySock):
    '''Code to handle a connection from some other system component'''
#    print "Received a connection."
    totalData = ''
    Done = False
    global HeavyLog
    global logFile

    if (HeavyLog): logFile = open(logFilePrefix + datetime.datetime.now().strftime("%y%m%d") + ".txt","a")
    data = ''
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
#        print data
#        print "Bytes received: " + str(len(totalData[1]))
        if '#' in data:  #The '#' character marks the end of data transmissions
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

            if (ParsedMessage[0] == "!TST"): #This is an instruction for a thermostat
                mySock.sendall(str(controlThermostat(ParsedMessage[2], ParsedMessage[3],True)))  #Function returns thermostat status
            elif (ParsedMessage[0] == "!TSU"): #This is an update from a thermostat
                controlThermostat(ParsedMessage[2], ParsedMessage[3],False)  #Function updates tstat and saves to database.  Thermostat does not expect a response at this time.
            elif (ParsedMessage[0] == "!TPU"): #This is a notice that a thermostat program has been updated which updates the current temperature
                mySock.sendall(updateTstatProgram(ParsedMessage[2], ParsedMessage[3]))
            elif (ParsedMessage[0] == "!TPT"): #This is a request for the current temperature under a given program
                returnProgramSetpoint(ParsedMessage[2], ParsedMessage[3])
            elif (ParsedMessage[0] == "!TSS"): #This is a request for thermostat status
                mySock.sendall(tstatStatusRequest(ParsedMessage[2], ParsedMessage[3]))
            else:  #This is a bad message type
                if (not HeavyLog): logFile = open(logFilePrefix + datetime.datetime.now().strftime("%y%m%d") + ".txt","a")
                logFile.write("Bad message type: " + totalData + "\n")
                logFile.write("Time:  " + datetime.datetime.now().strftime("%m/%d/%y %I:%M:%S %p") + "\n")
                logFile.write("_____________________\n")
                if (not HeavyLog): logFile.close();
                mySock.sendall("&" + ParsedMessage[0][1:] + "/" + ParsedMessage[1] + "/" + "0/" + "0\n")
    
            Done = True
#            print("Sending response")
            mySock.close()
            if (HeavyLog): logFile.close()
    

def updateTstatProgram(recipientID, program):
#   Find the correct thermostat and assign it to variable tstat
    for eachThermostat in Thermostats:
        if eachThermostat.port == int(recipientID, 10):
            tstat = eachThermostat
            break

    tstat.retrieveProgram(tstat.program)
#    print "tstat.override = " + tstat.override
    if tstat.override == "N": tstat.SearchForCurrEvent(datetime.datetime.now(), tstat.program)
    if tstat.override != "H": tstat.SearchForNextEvent(datetime.datetime.now(), tstat.program)
#    print "Next event: ", datetime.datetime.combine(tstat.NextEventDate, tstat.NextEventTime)
    print "-------------------------------"
    print "Updated program for tstat " + tstat.name + " and program " + program
    print "-------------------------------"
    return "Updated program for tstat " + tstat.name + " and program " + program



def returnProgramSetpoint(recipientID, program):
#   Find the correct thermostat and assign it to variable tstat
    for eachThermostat in Thermostats:
        if eachThermostat.port == int(recipientID, 10):
            tstat = eachThermostat
            break

    tstat.retrieveProgram(program)
    print "Program = " + program
    programSetpoint = tstat.SearchForCurrEvent(datetime.datetime.now(), program, False)
    print "programSetpoint = ", programSetpoint
    tstat.sendInstruction('U=' + str(int(programSetpoint)*10))



def controlThermostat(recipientID, instruction, sendToTstat):
    '''Get recipientID and instruction,
       parse and send instructions to recipient
       get resposne from recipient
       send response to original sender'''

    global logFile
    host = 'localhost'
    port = 11000+ int(recipientID,10)
    allData = ''
#    print "In the thermostat control routine."
#    print "Instruction: ", instruction

#   Find the correct thermostat and assign it to variable tstat
    for eachThermostat in Thermostats:
        if eachThermostat.port == int(recipientID, 10):
            tstat = eachThermostat
            break
#NOTE: THIS CODE ASSUMES THAT THE recipientID WILL BE FOUND.  IS THAT OKAY?
    prevOverride = tstat.override
#    print "prevOverride = " + prevOverride

#   Parse instruction into individual instructions using comma separator
    instructionList = instruction.split(",")

#   Send instructions to ultimate recipient
    for oneInstruction in instructionList:      
        if (oneInstruction == "A" or oneInstruction == "0" or oneInstruction == "1"):
            tstat.system = oneInstruction
            if (sendToTstat): tstat.sendInstruction(oneInstruction)
            if oneInstruction == "0": tstat.program = "H"  #Just a default value since tstat is OFF
        elif (oneInstruction[0]=="h"):
            tstat.mode = int(oneInstruction[2])
            if tstat.mode == 0: tstat.program = "C"
            else: tstat.program = "H"
            if (sendToTstat): tstat.sendInstruction(oneInstruction)
        elif (oneInstruction[0]=="f"):
            tstat.fan = oneInstruction[2]
            if (sendToTstat): tstat.sendInstruction(oneInstruction)
        elif (oneInstruction[0]=="s"):
#            print oneInstruction[-1], ",  ", sendToTstat
            tstat.override = oneInstruction[-1]
            if (sendToTstat):
                if (oneInstruction[-1]=="H"): tstat.sendInstruction("p=2")  #set the hold parameter
                elif (oneInstruction[-1]=="T"): tstat.sendInstruction("p=1")
                else: tstat.sendInstruction("p=0")
            if (oneInstruction[-1] != "N" and sendToTstat):
                tstat.setTemp(float(oneInstruction[2:-1]))  #override program temperature

    if (sendToTstat): tstat.sendInstruction("w")  #write the new thermostat instructions into EEPROM
    msg = "Instructions delivered "

#   Handle overrides
#    print "tstat.override = " + tstat.override
#    print "prevOverride = " + prevOverride
    if (tstat.override == "H" and prevOverride != "H"): tstat.NextEventDate = datetime.date(2099,12,31)
    if (tstat.override != "H" and prevOverride == "H"): tstat.SearchForNextEvent(datetime.datetime.now(), tstat.program)
    if (tstat.override == "N"): tstat.SearchForCurrEvent(datetime.datetime.now(), tstat.program)

#   Save thermostat status to the database
    if (tstat.saveStatusToDatabase()): msg += "and saved."
    else: msg += "but could not be saved."
    
    return msg
#    print msg
       


def tstatStatusRequest(tstatNodes, instruction):
    '''Get tstat status for one or more tstats.  For now, instruction has no meaning.
       tstatNodes is a list of comma separated values containing the nodes of the
       desired tstats'''

    #Create list of tstat nodes to cycle through
    tstatList = tstatNodes.split(',')
#    print "Will get status of thermostats:", tstatList
    result = ""
    for tstatNode in tstatList:
        result += tstatNode + "," + getTstatStatus(tstatNode) + ";"
        
    #remove the final trailing and return
    return result[:-1]



def getTstatStatus(tstatNode):
    '''Get tstat status for a single thermostat'''

    #Get the values from the database
    try:
#        db = pymysql.connect(unix_socket="/var/run/mysqld/mysqld.sock", user="saltemeier", passwd="ashwood", db="load_controller")
        db = pymysql.connect(unix_socket="/var/run/mysqld/mysqld.sock", user=MySQL_User, passwd=MySQL_Pwd, db=database_name)
    except:
        status = "-99,-99,-99,-99"
    else:
        c = db.cursor(pymysql.cursors.DictCursor)
        query = "SELECT * FROM thermostat_status WHERE userID = " + string(user_id) + " AND port = " + tstatNode + " AND " + \
                "saved = (SELECT MAX(saved) FROM thermostat_status WHERE userID = " + string(user_id) + " AND port = " + tstatNode + ")"
        c.execute(query)
        result = c.fetchone()
#        print result
        c.close()
        db.close()
        status = result['system'] + "," + str(result['mode']) + "," + str(result['fan']) + "," + result['override']
    
    #Get the values from the thermostat
    setTemp = sendInstruction(tstatNode,"? s")
    if ("not" in setTemp):
        setTemp = "-99"
    else:
        try:
            setTemp = str(float(setTemp[setTemp.find("=")+1:])/10)
        except ValueError:
            setTemp = "-99"
    status += "," + setTemp

    actTemp = sendInstruction(tstatNode,"? t")
    if ("not" in actTemp):
        actTemp = "-99"
    else:
        try:
            actTemp = str(float(actTemp[actTemp.find("=")+1:])/10)
        except ValueError:
            actTemp = "-99"
    status += "," + actTemp

    return status



def sendInstruction(recipientID, instruction):
#def sendInstruction(recipientID, msgType, instruction):
    '''Get recipientID and instruction,
       send instruction to recipient
       get resposne from recipient
       send response to original sender'''

    global logFile
    host = 'localhost'
    port = 11000+ int(recipientID,10)
    allData = ''

    instruction += "\n"
#    print "Received instruction: " + instruction
    
#   Send instruction to ultimate recipient
    try:
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.connect((host,port))
    except socket.error as msg:
        if client:
            client.close()
        logFile = open(logFilePrefix + datetime.datetime.now().strftime("%y%m%d") + ".txt","a")
        logFile.write("Time:  " + datetime.datetime.now().strftime("%m/%d/%y %I:%M:%S %p") + "\n")
        logFile.write("Could not connect to port " + str(port) + " to send current status instruction.")
        logFile.write("Error no. " + msg[0] + ": \t" + msg[1])
        logFile.write("_____________________\n")
        logFile.close()
#        print "Could  not connect to port " + str(port)
        return "Could not connect to port " + str(port)
    else:
        client.send(instruction)

    print "Sent instruction " + instruction 
    data = ""
    timeout = time.time() + 10  #Timeout after 10 seconds
#        print "time is " + str(time.time()) + " and timeout is " + str(timeout)
    while ('#' not in data) and (time.time()<timeout) and ('\r' not in data):  #'\r' is for temp status and '#' is for others
#            print "time is now " + str(time.time())
        try:
            avail, _, _ = select.select([client], [], [], 0)
            if avail:
                data2 = client.recv(256)
                data += data2
#                    print "data received is " + data
        except socket.error as (value, msg):
            if client:
                client.close()
            return "Did not receive a response from port " + str(port)
                
    print "Received response from LC: " + data
    client.close()
    if ('\r' in data):
        data = data[:data.find('\r')]
    return data
        

#############################################################################
#Main program code starts here

#Get a list of thermostats
newdb = pymysql.connect(unix_socket="/var/run/mysqld/mysqld.sock", user=MySQL_User, passwd=MySQL_Pwd, db=database_name)
newc = newdb.cursor()
newc.execute("SELECT name, node FROM thermostats WHERE user_id = " + string(user_id))
tstatList = newc.fetchall()

#Create a Thermostat List
Thermostats = []
count = 0;
for tstat in tstatList:
    #Retrieve current status for each thermostat and load program
    newc.execute("SELECT * FROM thermostat_status WHERE userID = " + string(user_id) + " AND port = " + str(tstat[1]) + " AND " + \
                 "saved = (SELECT max(saved) FROM thermostat_status WHERE userID = " + string(user_id) + " AND port = " + str(tstat[1]) + ")")
    status = newc.fetchone()
#if OFF, no porogram; if mode = 1 cool progrqm else heat program; if override = T or H, set temp to override; if override = H, set nextEventDate
    Thermostats.append(Thermostat.Thermostat(tstat[0], tstat[1], status))

#Save message indicating restart to log file
logFile = open(logFilePrefix + datetime.datetime.now().strftime("%y%m%d") + ".txt","a")
logFile.write("\nTstatMaster restart: " + datetime.datetime.now().strftime("%m/%d/%y %I:%M:%S") + "\n\n")
logFile.write("_____________________\n")
logFile.close()

newdb.close()
    

#start listening forever
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.bind((host,serverPort))
s.listen(backlog)

print "running " + sys.argv[0][sys.argv[0].rfind('/')+1:]
print "serving at port ", serverPort
while 1:  #Listen forever and check thermostat programs
    ready_to_read, ready_to_write, in_error = select.select([s],[s],[],0)
    if (ready_to_read):
        client, address = s.accept()
        SimpleHandler(client)

    #This is the thermostat timer code
    now = datetime.datetime.now()
    for eachThermostat in Thermostats:
        eachThermostat.Timer(now)
    
