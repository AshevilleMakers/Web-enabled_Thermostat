<?php # Code to retrieve current system status
$page_title = 'Current Status';
$CommPwd = '4321';
session_start();
if (!isset($_SESSION['userID']))
{
    header('Location: /tstat_login.php');
    exit;
}

$userID = $_SESSION['userID'];
$userName = $_SESSION['username'];
date_default_timezone_set('America/New_York');
$address = 'localhost';
$service_port = '1800';
$thermostats = ['105','112'];  //Enter nodes for your thermostats here

include ($_SERVER['DOCUMENT_ROOT'] . '/includes/tstat_MainHeader.php');

echo <<<_END
        <style>
            div {
                position:   absolute;
                height:     300px;  }
                width:      300px;
                top:        150px;
            #Tstats {
                width:      600px;
                top:        470px;
                left:       5px; }

        </style>
_END;

//Connect to the database
require_once(dirname(dirname(__FILE__)) . '/private/tstat_ConnDb.php');


//Get the thermostat data and display it on this page
//Create a socket
$socket = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
if ($socket === false) {
    echo "socket_create() failed: reason: " . socket_strerror(socket_last_error()) . "\n";
} else {
//    echo "Socket created.<BR>";
}

//Connect to the socket
$result = socket_connect($socket, $address, $service_port);
if ($result === false) {
    echo "socket_connect() failed.\nReason: ($result) " . socket_strerror(socket_last_error($socket)) . "\n";
} else {
//    echo "Socket connected<br>";
}

//Create and send the instruction to the server
//Get a list of all thermostats
//This should come from a query once all thermostats are installed
//Maybe make a lookup function for all pages to use - see PHP array_search
//See also this http://stackoverflow.com/questions/6661530/php-multi-dimensional-array-search
$instruction = "!TSS/" . $CommPwd . "/" . implode(",", $thermostats) . "/junk#\n";
socket_write($socket, $instruction, strlen($instruction));

//Receive a response
$totResponse = "";
while ($response = socket_read($socket, 2048)) {
    $totResponse = $totResponse . $response;
}

//Close the socket
socket_close($socket);

//Get the thermostat names
$query = "SELECT node, name FROM thermostats WHERE user_id = '" . $userID . "'";

if ($result = mysqli_query($dbc, $query)) {
    $thermostats = [];
    while ($row = mysqli_fetch_assoc($result)) {
        $thermostats[] =  $row;
   }
}


echo <<<_END
    <div id='Tstats'><b>Thermostats</b><table>
        <tr>
            <td><b>Name</b></td>
            <td align="right"><b>System</b></td>
            <td align="right"><b>Fan</b></td>
            <td align="right"><b>&nbsp &nbsp &nbsp Override</b></td>
            <td align="right"><b>&nbsp &nbsp &nbsp Set Temp</b></td>
            <td align="right"><b>&nbsp &nbsp &nbsp Room Temp</b></td>
        </tr>
_END;

//Parse the thermostat results
//Results are on the format port/system,mode,fan,override,set temp,room temp;
$allTstatData = explode(";", $totResponse);
for ($i=0; $i<count($allTstatData); $i++) {
    $oneTstatData = explode(",", $allTstatData[$i]);
    $name = FindTstatName($oneTstatData[0], $thermostats);
    echo "<tr>";
    echo "<td>$name</td>";  //System is Off
    if  ($oneTstatData[1] == 0) {
        echo '<td align="right">Off</td>';
    }
    else {  //System is On
        if  ($oneTstatData[2] == 0) {  //Cooling mode
            echo '<td align="right">Cool</td>';
        }
        elseif  ($oneTstatData[2] == 1) {  //Heating mode
            echo '<td align="right">Heat</td>';
        }
        elseif  ($oneTstatData[2] == 2) {  //Emergency heat mode
            echo '<td align="right">Em Heta</td>';
        }
        else {  //Auxilliary heat  mode
            echo '<td align="right">Aux Heat</td>';
        }
    }

    if ($oneTstatData[3] == 0) { //Fan is Auto
        echo '<td align="right">&nbsp &nbsp &nbspAuto</td>';
    }
    else {
        echo '<td align="right">&nbsp &nbsp &nbsp On</td>';
    }

    if ($oneTstatData[4] == 'N') { //No override - running program
        echo '<td align="right">None</td>';
    }
    elseif ($oneTstatData[4]== 'T') {  //Temporary override
        echo '<td align="right">Temporary</td>';
    }
    else  {  //Indefinite
        echo '<td align="right">Hold</td>';
    }

    for ($j=5; $j<count($oneTstatData); $j++) {
        echo '<td align="right">' . $oneTstatData[$j] . '</td>';
    }
    echo "</tr>";
}
echo "</table></div>";
mysqli_close($dbc);

function FindTstatName($value, $array) {
    foreach ($array as $key => $val) {
        if ($val['node'] == $value) {
            return $val['name'];
        }
    }
    return $value;
}

?>
    </body>
</html>
