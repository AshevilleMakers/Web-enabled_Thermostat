import datetime
import socket
import pymysql

#Error handling is virtually non-existant in this code right now

class Thermostat:
    '''This is a class for thermostats'''


    def __init__(self, _name, _port, status): #, _status, _mode, _heatTimes=[], _heatTemps=[], _coolTimes=[], _coolTemps=[]):
#        self.weekdays = _weekdays
        #Use conprehension to convert strings into times
#        self.times = [[datetime.datetime.strptime(time, '%H:%M:%S').time() for time in timerow] for timerow in _times]
#        self.temps = _temps
        self.port = _port
        self.name = _name
        self.initialize(status)
#       Other attributes include:
#           self.nextEventDate
#           self.nextEventTime
#           self.nextTemp
#           self.CurrTemp
#           self.heatTimes
#           self.heatTemps
#           self.heatWeekdays
#           self.coolTimes
#           self.coolTemps
#           self.coolWeekdays
#           self.status
#           self.mode (0 through 3)
#           self.program = "H" or "C" for heating or cooling program
#           self.override
#           self.fan


    def Timer(self, now):
        '''Compares current datetime to NextEventDate and NextEventTime and set the new temperature as needed'''

        #Compare current date and time to next event date and time.  If greater, execute event and get next event.
        if now > datetime.datetime.combine(self.NextEventDate, self.NextEventTime):
            #Send message to thermostat
            print "--------------------------------"
            print now
            print "Thermostat: " + self.name
            if self.override != "N":
                self.override = "N"
#                self.sendInstruction("P=N")  #let thermostat know back on the program now
                self.saveStatusToDatabase()
            self.setTemp(self.NextTemp)
            self.sendInstruction("w")    #make sure thermostat saves the changes
            self.SearchForNextEvent(now, self.program)

        
    def SearchForNextEvent(self, now, program):
        '''Sets the NextEventDate and NextEventTime atrributes: takes no action'''

        currDate = now.date()
        currTime = now.time()

      # python weekday function returns 0 for Monday through 6 for Sunday (load controller uses 0 for Sunday through 6 for Saturday)
        weekday = datetime.datetime.weekday(now)
        if (weekday==7): weekday = 0

        NextEventDate = currDate
        NextEventWeekdaysColumn = weekday
        NextEventTime = -1

        #Find a time on the current date that is later than the current time
        Counter = 0
        if program == "H":
            tempWeekdays = self.heatWeekdays
            tempTimes = self.heatTimes
            tempTemps = self.heatTemps
        else:
            tempWeekdays= self.coolWeekdays
            tempTimes = self.coolTimes
            tempTemps = self.coolTemps

        TempIndex = tempWeekdays[NextEventWeekdaysColumn]  #This gives the row of timers and temps to use
        for time in tempTimes[TempIndex]:
            if time > currTime:
                NextEventTime = time
                NextTemp = tempTemps[TempIndex][Counter]
                break
            Counter += 1


        #If did not find, find next valid time on any date
        while NextEventTime == -1:
            #Move to the next day
            NextEventDate = NextEventDate + datetime.timedelta(days=1)  #AddDays(lNextEventDate, 1)
            NextEventWeekdaysColumn+=1
            if NextEventWeekdaysColumn==7: NextEventWeekdaysColumn = 0
            TempIndex = tempWeekdays[NextEventWeekdaysColumn]  #This gives the row of timers and temps to use
            for time in tempTimes[TempIndex]:
                if time:  #Will only be false is there is no time value in this row other than an intial 0
                    NextEventTime = time
                    NextTemp = tempTemps[TempIndex][0]  #Will always be the first temp on a new day
                    break

            
        self.NextEventDate = NextEventDate
        self.NextEventTime = NextEventTime
        self.NextTemp = NextTemp


    def SearchForCurrEvent(self, now, program):
        '''This checks the programming to find what temp the thermostat should be in now and sets that new temp'''

        currDate = now.date()
        currTime = now.time()

      # python weekday function returns 0 for Monday through 6 for Sunday (load controller uses 0 for Sunday through 6 for Saturday)
        weekday = datetime.datetime.weekday(now)
        if (weekday==7): weekday = 0

        CurrEventDate = currDate
        CurrEventWeekdaysColumn = weekday
        CurrEventTime = -1

        #Find a time on the current date that is earlier than the current time
        Counter = 0
        if program == "H":
            tempWeekdays = self.heatWeekdays
            tempTimes = self.heatTimes
            tempTemps = self.heatTemps
        else:
            tempWeekdays= self.coolWeekdays
            tempTimes = self.coolTimes
            tempTemps = self.coolTemps

        TempIndex = tempWeekdays[CurrEventWeekdaysColumn]  #This gives the row of timers and temps to use
        for time in reversed(tempTimes[TempIndex]):
            if time <= currTime:
                CurrEventTime = time
                NumData = len(tempTemps[TempIndex])
                CurrTemp = tempTemps[TempIndex][NumData - Counter - 1]  #Counter is counting from the end in this case
                break
            Counter += 1


        #If did not find, find next valid time on any earlier date
        while CurrEventTime == -1:
            #Move to the previous day
            CurrEventDate = CurrEventDate - datetime.timedelta(days=1)
            CurrEventWeekdaysColumn -= 1
            if CurrEventWeekdaysColumn == -1: CurrEventWeekdaysColumn = 6
            TempIndex = tempWeekdays[CurrEventWeekdaysColumn]  #This gives the row of timers and temps to use
            for time in reversed(tempTimes[TempIndex]):  #Reverse since want last time on previous day
                if time:  #Will only be false is there is no time value in this row other than an intial 0
                    CurrEventTime = time
                    NumData = len(tempTemps[TempIndex])
                    CurrTemp = tempTemps[TempIndex][NumData - 1]  #Will always be the last temp on a new day
                    break

#        print "Current temp setting: ", CurrTemp
        self.setTemp(CurrTemp)
#        self.SearchForNextEvent(now, mode)


    def sendInstruction(self, instruction):
        '''Send a single instruction to the thermostat'''
        host = 'localhost'
        try:
            client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            client.connect((host,11000+self.port))
        except socket.error as msg:
            if client:
                client.close()
#                logFile.write("Time:  " + datetime.datetime.now().strftime("%m/%d/%y %I:%M:%S %p") + "\n")
#                logFile.write("Could not connect to port " + str(port) + " to send current status instruction.")
#                logFile.write("Error no. " + msg[0] + ": \t" + msg[1])
#                logFile.write("_____________________\n")
                print "Could not connect to thermostat at port " + str(self.port)
                return False
        else:
#Add the following 3 lines back in after testing
            client.send(instruction + "\n")
#            client.send("w\n")
            client.close()
            return True


    def setTemp(self, newTemp):
        '''Sets the new temperature of the Thermostat'''
        if self.system == 0: return  #heat pump is in manual_override OFF
        instruction = 's=' + str(int(newTemp*10))
        if (self.sendInstruction(instruction)):
            self.CurrTemp = newTemp
            print "Sent instruction: " + instruction
            print "--------------------------------"

        if self.override=="N": Hold="0"
        elif self.override=="T": Hold="1"
        elif self.override=="H": Hold="2"
        else: print "Error: Unknown self.override = " + self.override
        instruction = 'P=' + Hold
        if (self.sendInstruction(instruction)):
            print "Sent instruction: " + instruction
            print "--------------------------------"

        self.sendInstruction("w")


    def retrieveProgram(self, program):
        '''Retrieves the thermostat program from the database; mode is either 'H' for heating program or 'C' for cooling program (or defaujlt to cooling if neither) but takes no action'''
        #After retrieving program, still need to SearchForCurrEvent and SearchForNextEvent if not in override
    
        #connect to the databases
        db = pymysql.connect(unix_socket="/var/run/mysqld/mysqld.sock", user="saltemeier", passwd="ashwood", db="load_controller")
        c = db.cursor(pymysql.cursors.DictCursor)

        #retrieve thermostat program
        if program =='H' or program == 'h': program = "H"
        else: program = "C"
    
        query = ("SELECT * from thermostat_programs WHERE userID='123456' AND port='" + str(self.port) +
                 "' AND heat_cool='" + program + "' AND saved = (SELECT max(saved) FROM thermostat_programs "
                 "WHERE userID='123456' AND port='" + str(self.port) + "' AND heat_cool='" + program + "');")
        result = c.execute(query)
        record = c.fetchone()
#        print "Retrieved program record: ", record 

        if (result):
            if program == "H":
                self.heatTimes = []
                self.heatWeekdays = []
                self.heatTemps = []
                self.heatWeekdays = [int(val) for val in record['weekdays'].split(',')]
                #split times record by "/"; then split each record by "," and convert times
                temp = record['times'].split('/')
#                print "Times record: ", tempTime
                for timeRow in temp:
                    self.heatTimes.append([datetime.datetime.strptime(val, '%H:%M').time() for val in timeRow.split(',')])
#                    print self.heatTimes
                temp = record['temps'].split('/')
#                print "Temps record: ", tempTemp
                for tempRow in temp:
                    self.heatTemps.append([float(val) for val in tempRow.split(',')])
#                    print self.heatTemps
            else:
                self.coolTimes = []
                self.coolWeekdays = []
                self.coolTemps = []
                self.coolWeekdays = [int(val) for val in record['weekdays'].split(',')]
                #split times record by "/"; then split each record by "," and convert times
                temp = record['times'].split('/')
                for timeRow in temp:
                    self.coolTimes.append([datetime.datetime.strptime(val, '%H:%M').time() for val in timeRow.split(',')])
                temp = record['temps'].split('/')
                for tempRow in temp:
                    self.coolTemps.append([float(val) for val in tempRow.split(',')])
        else:
            print "Query to retrieve program failed: ", query
            return

        #Print program results for testing
#        print "Port: ", self.port
#        if mode == "H":
#            print "Weekdays: ", self.heatWeekdays
#            print "Times: ", self.heatTimes
#            print "Temps: ", self.heatTemps
#        else:
#            print "Weekdays: ", self.coolWeekdays
#            print "Times: ", self.coolTimes
#            print "Temps: ", self.coolTemps


    def saveStatusToDatabase(self):
        try:
            db = pymysql.connect(unix_socket="/var/run/mysqld/mysqld.sock", user="saltemeier", passwd="ashwood", db="load_controller")
        except:
            return false
        else:
            c = db.cursor()
            currDateTime = datetime.datetime.now().strftime("%y-%m-%d %H:%M:%S")
            query = "INSERT INTO thermostat_status VALUES ('123456','" + str(self.port) + "','" + currDateTime + "','" + self.system + "','" + str(self.mode) + "','" + self.fan + "',"
#            print query
            if self.override != "N":
                query += "'" + str(self.CurrTemp) + "','"
#                print query
            else:
                query += "NULL,'"
#                print query
            query += self.override + "')"
#            print query
    
            c.execute(query)
            db.commit()
            c.close()
            db.close()
            return True
        

    def initialize(self, status):
        '''Initialize a new thermostat.  Status contains a record from table thermostat_status.'''

#        print "Initializing the " + self.name + " thermostat."
#        print "Status = ", status

        #Set the fan since this must be done regardless of all other settings
        self.fan = status[5]
        self.sendInstruction("f=" + str(self.fan))
        
        #Set the system variable and exit if system is OFF
        self.system=status[3]
        self.sendInstruction(self.system)
#        if self.system == "0":
#            self.sendInstruction("w")
#            return #Nothing else to do since thermostat is off

        #Set self.mode and send the "h=" instruction and retrieve the program (assumes always a program)
        self.mode = status[4]
        self.sendInstruction("h=" + str(self.mode))
        if (self.mode == 0): self.program="C"
        else: self.program="H"
#        print "Mode: ", self.mode 
#        self.retrieveProgram(self.program)
        self.retrieveProgram("H")
        self.retrieveProgram("C")

        #Check for an override
        self.override = "N"  #N for NONE is the default value
#        print "System: ", self.system
        if status[6] != None and self.system != "0":  #there is a set temperature
#            print "Here"
            self.override = status[7]
            if (self.override=="T"): self.sendInstruction("P=1")
            else: self.sendInstruction("P=2")
            self.setTemp(float(status[6]))
            if self.override=="H":  #hold the current setting
                self.NextEventDate=datetime.date(2099,12,31)
                self.NextEventTime = datetime.time(23,59,59)
                self.sendInstruction("w")
                return
        else:  #there is no override so set the current temp from the program
#            print "There"
            self.sendInstruction("P=0")
            self.SearchForCurrEvent(datetime.datetime.now(), self.program)

        #Search for the next event and wrap it up
        self.SearchForNextEvent(datetime.datetime.now(), self.program)
        self.sendInstruction("w")  #Save the current status in the thermostat

