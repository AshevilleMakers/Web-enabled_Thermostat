<?php # Code to program thermostat
$page_title = 'Thermostat Program';
$address = 'localhost';
$service_port = '1800';  //Port on which the control TstatMaster.py server runs
$CommPwd = '4321';  //4 digit user-selected PIN for the rfx communication (also in arduino sketch and python servers)
date_default_timezone_set('America/New_York');  //Set to your default time zone

session_start();
if (!isset($_SESSION['userID']))
{
    header('Location: /private/tstat_login.php');
    exit;
}

$userID = $_SESSION['userID'];
$userName = $_SESSION['username'];

if (!isset($programTimes)) {
    $programTimes = array_fill(0, 10, array_fill(0, 10, ""));  //6 possible time/temp pairs for each of 7 days
    $programTemps = array_fill(0, 10, array_fill(0, 10, ""));
}

include ($_SERVER['DOCUMENT_ROOT'] . '/includes/tstat_MainHeader.php');

//Connect to the database
require_once(dirname(dirname(__FILE__)) . '/private/tstat_ConnDb.php');

$query = "SELECT node, name FROM thermostats WHERE user_id = '$userID'";

if ($result = mysqli_query($dbc, $query)) {
    $j = 0;
    while ($row = mysqli_fetch_assoc($result)) {
        $thermostats[$j][0] = $row['node'];
        $thermostats[$j][1] = $row['name'];
//        echo "Name: " . $relaynames[$j][2] . "   Index: " . $relaynames[$j][1] . "<br>";
        $j += 1;
    }
}

if (isset($_POST['tsindex'])) {
    $tsindex = $_POST['tsindex'];
}
else {
    $tsindex = 0;  //This is the default value to be selected when page first opened
}
$tsnode = $thermostats[$tsindex][0];
$tsname = $thermostats[$tsindex][1];

if (isset($_POST['system'])) {
    $system = $_POST['system'];
}
else {
    $system = "C";  //This is the default value to be selected when page first opened
}

for ($i=0;$i<7;$i++) {  //Loop through each day
    if (isset($_POST["dataTimes$i"])) {
        $userTimes = $_POST["dataTimes$i"];
        $userTemps = $_POST["dataTemps$i"];
        foreach ($userTimes as $index=>$time) {
            $programTimes[$index][$i] = $time;
            $programTemps[$index][$i] = $userTemps[$index];
        }
    }
}

//If user has requested to retrieve current program, rettrieve program
if (isset($_POST["Retrieve"])) {  //Code to get current setting sp can fill matrix
    $query = "SELECT times, temps, weekdays from thermostat_programs WHERE userID='" . $userID . "' AND port='" . $tsnode .
             "' AND heat_cool='" . $system . "' AND saved = (SELECT max(saved) FROM thermostat_programs " .
             "WHERE userID='" . $userID . "' AND port='" . $tsnode . "' AND heat_cool='" . $system . "');";
//    echo "$query<br>";

    if ($result = mysqli_query($dbc, $query)) {
        while ($row = mysqli_fetch_assoc($result)) {
            $times = $row['times'];
            $temps = $row['temps'];
            $weekdays = $row['weekdays'];
        }
    }

    $weekdaysArray = explode(",", $weekdays);
    $tempTimes = explode("/", $times);  //This is now array of strings, one string for each day
    $tempTemps = explode("/", $temps);  //This is now array of strings, one string for each day
    $timesArray = [];
    $tempsArray = [];
    foreach ($tempTimes as $timeRow) {
        $timesArray[] = explode(",", $timeRow);
    }
    foreach ($tempTemps as $tempRow) {
        $tempsArray[] = explode(",", $tempRow);
    }

    //Now fill the table
    foreach ($timesArray as $index=>$times) {  //This is the day index
        foreach ($weekdaysArray as $col=>$weekday) {  //Loop through weekdays looking for this day index
            if ($weekday==$index) {  //Match so populate this weekday in the table
                foreach ($times as $row=>$time) {  //This is the index for times and temps within the day
                    $programTimes[$row][$col] = $time;
                }
            }
        }
    }
    foreach ($tempsArray as $index=>$temps) {  //This is the day index
        foreach ($weekdaysArray as $col=>$weekday) {  //Loop through weekdays looking for this day index
            if ($weekday==$index) {  //Match so populate this weekday in the table
                foreach ($temps as $row=>$temp) {  //This is the index for times and temps within the day
                    $programTemps[$row][$col] = $temp;
                }
            }
        }
    }
}

?>

<script>
    function confirmSubmit() {
        return confirm("Press OK to confirm you want to save the program data.");
    }

    function clearDays() {
        for (var i=0;i<7;i++) {
            myForm.elements['weekdays[]'][i].checked=false;
        }
    }

    function checkDays() {
        //This function checks multiple days when a radio button is selected
        clearDays();
        if (myForm.elements['days'][0].checked) {
            for (var i=0;i<5;i++) {
                myForm.elements['weekdays[]'][i].checked=true;
            }
        }
        else if (myForm.elements['days'][1].checked) {
            for (var i=5;i<7;i++) {
                myForm.elements['weekdays[]'][i].checked=true;
            }
        }
        else if (myForm.elements['days'][2].checked) {
            for (var i=0;i<7;i++) {
                myForm.elements['weekdays[]'][i].checked=true;
            }
        }
    }

    function updateProg() {
        //This is a script to update the table of times and temps when the Update button is clicked
        //The info goes in the arrays $programTimes and $programTemps to be sent later
        //The info is displayed in the table program
        //This could be a place for validation using a validation function
        //Step 1: Loop through each weekday and apply the data to selected weekdays

        var myElement = myForm.elements['weekdays[]'];
        var weekdays = new Array();
        for (var i=0; i<myElement.length; i++) {
            weekdays[i] = myElement[i].checked;
        }

        myElement = myForm.elements['times[]'];
        var times = new Array()
        for (var i = 0; i < myElement.length; i++) {
            times[i] = myElement[i].value;
        }

        myElement = myForm.elements['temps[]'];
        var temps = new Array()
        for (var i = 0; i < myElement.length; i++) {
            temps[i] = myElement[i].value;
 //           alert("temps[" + i + "] = " + temps[i])
        }

        //Update the table program based on this info
        myForm = document.getElementById("myForm");
        //Loop through the weekdays (columns)
        for (var i=0; i<weekdays.length; i++) {
            if (weekdays[i]==true) {
                //Loop through the time and temp values - assume for now they are legitimate values
                for (var j=0; j<times.length; j++) {
                    if (times[j]) {  //Ignore the empty cells
                        myForm.elements['dataTimes' + i + '[]'][j].value = times[j];
                        myForm.elements['dataTemps' + i + '[]'][j].value = temps[j];
                    }
                }
            }
        }
    }

</script>

<p>
<!--    <form id="myForm" action="tstat_ProgTstat.php" method="post" onsubmit="return confirmSubmit();"> -->
    <form id="myForm" action="tstat_ProgTstat.php" method="post">
    Select Thermostat<br/>
<?php
    $j = 0;
    foreach ($thermostats as $namerow) {

        $checked = '';
        if ($j == $tsindex) {$checked = ' checked="checked"';}
        echo '<input type="radio" name="tsindex" value="' . $j . '"' . $checked . '> ' . $namerow[1] . ' </input> &nbsp;&nbsp;&nbsp;&nbsp;';
        $j+=1;
    }
?>
    <br></br>
    Select Heating or Cooling<br/>
    <input type="radio" name="system" value="C" <?php if ($system == "C") echo "checked='checked'"; ?> >COOL &nbsp;&nbsp;&nbsp;&nbsp;
    <input type="radio" name="system" value="H" <?php if ($system == "H") echo "checked='checked'"; ?> >HEAT <br></br>

    <input type = "submit" name="Submit" value="Submit" style="width:100px; height:30px" onclick="return confirmSubmit();"></input>
    <input type = "submit" name="Retrieve" value="Retrieve" style="width:100px; height:30px"></input>

</p>

<br>
    <table id="progInputs" border="3">
        <tr height="16"><th width="75">Time</th><th width="75">Temp</th></tr>
<?php
        for ($i = 0; $i < 6; $i ++) {  //6 rows to allow for 6 times per day for now
            echo "<tr><td><input type='text' name='times[]' size='10'></input></td>";
            echo "<td><input type='text' name='temps[]' size='10'></input></td></tr>";
        }
 ?>
   </table><br></br>

<?php
    $weekdayNames = ['Monday','Tuesday','Wednesday','Thursday','Friday','Saturday','Sunday'];
    for ($i=0; $i<7;$i++) {
        echo '<input type="checkbox" name="weekdays[]"></input>' . $weekdayNames[$i] . ' &nbsp;&nbsp;&nbsp;&nbsp;';
    }
 ?>

    <input type="button" name="update" value="Update" style="width:100px; height:30px" onclick="updateProg();"></input> &nbsp;&nbsp;&nbsp;&nbsp;
    <input type="radio" name="days" value="weekdays" onclick="checkDays();">Mon.-Fri.</input> &nbsp;&nbsp;&nbsp;&nbsp;
    <input type="radio" name="days" value="weekend" onclick="checkDays();">Sat.-Sun.</input> &nbsp;&nbsp;&nbsp;&nbsp;
    <input type="radio" name="days" value="alldays" onclick="checkDays();">Mon.-Sun.</input> &nbsp;&nbsp;&nbsp;&nbsp;
    <input type="radio" name="days" value="clear" onclick="checkDays();">Clear</input>
    <br></br>


<!-- This is code to show the entered programming -->
<table id="program" border="3">
    <tr height="16">
    <?php
    for ($i=0;$i<7;$i++) {
        //Create headers with the weekdays in them
        echo "<th width='75'>$weekdayNames[$i]</th><th width='75'></th>";
    }
    echo "</tr>";
    //Create the table showing $programTimes and $programTemps
    for ($row=0;$row<6;$row++) {
        echo "<tr>";
        for ($col=0;$col<7;$col++) {
            echo '<td><input type="text" name="dataTimes' . $col. '[]" value="' . $programTimes[$row][$col] . '" size="12" maxlength="12"></input></td>';
            echo '<td><input type="text" name="dataTemps' . $col. '[]" value="' . $programTemps[$row][$col] . '" size="12" maxlength="12"></input></td>';
        }
        echo "</tr>";
    }
     ?>
</table>
</form>
<body>


<?php  //Code to send the entered instructions to the thermostat and the database

if (isset($_POST['Submit'])) {
//  Step 1: Get the programming data into PHP
//  Step 2: Save the programming data into the database
//  Step 3: Send the programming data to the thermostat server so it can be added to the thermostat objects
//          Should I send all 7 days separately, or try to remove overlapping days for efficiency?
//          Start by sending all 7 days individually even if some are identical
//          Later, could save some sort of index with the days to identify those that are identical

    if ($userName != "guest") {

        //Create a weekdays string (for now will  be 0,1,2,3,4,5,6).  String tells which row to use for each weekday
        $weekdayString = "0,1,2,3,4,5,6";

        //Create a time string and a temp string
        $timeString = $tempString = "";
        for ($i=0;$i<7;$i++) {  //Loop through each day
            $times = $_POST["dataTimes$i"];
            $temps = $_POST["dataTemps$i"];
            foreach ($times as $index=>$time) {
                if ($time!="") {  //This could be a place for validation using a validation function
                    $timeString .= $time . ",";
                    $tempString .= $temps[$index] . ",";
                }
            }
            //Now replace the last ',' with a '/' to delineate records
            $timeString = substr($timeString, 0, -1) . "/";
            $tempString = substr($tempString, 0, -1) . "/";
        }
        //Now remove the final '/'
        $timeString = substr($timeString, 0, -1);
        $tempString = substr($tempString, 0, -1);

        //Build a query string
        $query = "INSERT INTO thermostat_programs VALUES('" . $userID . "','";
        $tstatIndex = $_POST['tsindex'];
        $startDate = '2000-01-01';  //Since just want month and day, always use the year 2000
        $stopDate = '2000-12-31';
        $tsnode = $thermostats[$tstatIndex][0];
        $query .= $tsnode . "','" . $startDate . "','" . $stopDate . "','";
        $heatCool = $_POST['system'];
        $query .= $heatCool . "','" . $weekdayString . "','" . $timeString . "','" . $tempString . "','";
        $now = date('Y-m-d H:i:s');
        $query .= $now . "');";

        if ($result = mysqli_query($dbc, $query)) {
echo <<<_END
            <script>
                alert("Data saved")
            </script>
_END;
        }
        else {
echo <<<_END
            <script>
                alert("Data was not saved")
            </script>
_END;
        }

        //Send a message to the thermostat controller letting it know that the database has a new program
        //Send the timestamp in the message to make retrieval simple?  No, because won't have this for
        //other retrievals such as on startup after a power failure
        //Message is "P_" + port number for thermostat ("P" for program)

        $instruction = "!TPU/" . $CommPwd ."/" . $tsnode . "/" . $heatCool . "#\n";

        //Create a socket
        $socket = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
        if ($socket === false) {
            echo "socket_create() failed: reason: " . socket_strerror(socket_last_error()) . "\n";
        }

        //Connect to the server
        $result = socket_connect($socket, $address, $service_port);
        if ($result === false) {
            echo "socket_connect() failed.\nReason: ($result) " . socket_strerror(socket_last_error($socket)) . "\n";
        }

        //Send the instruction to the server
        socket_write($socket, $instruction, strlen($instruction));

        //Receive a response
        $totResponse = "";
        while ($response = socket_read($socket, 2048)) {
            $totResponse = $totResponse . $response;
        }
        echo "<br>";

        //Close the socket
        socket_close($socket);

    }
    else { //$userName == "guest"
echo <<<_END
    <script>
        alert("Guests are not allowed to change the thermostat programming.");
    </script>
_END;
    }
}
 ?>
</html>
