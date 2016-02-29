<?php # Code to set thermostat
$page_title = 'Set Thermostat';
$address = 'localhost';
$service_port = 1800;
$CommPwd = '4321';
date_default_timezone_set('America/New_York');

session_start();
if (!isset($_SESSION['userID']))
{
    header('Location: /tstat_login.php');
    exit;
}

$userID = $_SESSION['userID'];
$userName = $_SESSION['username'];

include ($_SERVER['DOCUMENT_ROOT'] . '/includes/tstat_MainHeader.php');

//Connect to the database
require_once(dirname(dirname(__FILE__)) . '/private/tstat_ConnDb.php');

$query = "SELECT node, name FROM thermostats WHERE user_id = '$userID'";

if ($result = mysqli_query($dbc, $query)) {
    $j = 0;
    while ($row = mysqli_fetch_assoc($result)) {
        $thermostats[$j][0] = $row['node'];
        $thermostats[$j][1] = $row['name'];
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

//Get the current thermostat settings from the database if not submitting
$query = "SELECT * FROM thermostat_status WHERE userID = '$userID' AND port = '$tsnode' AND saved = (SELECT MAX(saved) FROM thermostat_status WHERE userID = '$userID' AND port = '$tsnode')";
if ((!isset($_POST['submitted']) || $_POST['submitted']=="false") && ($result = mysqli_query($dbc, $query))) {
    while ($status = mysqli_fetch_assoc($result)) {
//        print_r($status);
        switch ($status['system']) {
            case "0":
                $system = "Off";
                break;
            default:
                switch ($status['mode']) {
                    case "0":
                        $system = "Cool";
                        break;
                    case "1":
                        $system = "Heat";
                        break;
                    case "2":
                        $system = "EmHeat";
                        break;
                    case "3":
                        $system = "AuxHeat";
                        break;
                }
                break;
        }
        if ($status['fan'] == "0") $fan = "Auto";
        else $fan = "On";
        $override = $status['override'];
        $newTemp = $status['temp'];
    }
}
else {
    if (isset($_POST['system'])) {
        $system = $_POST['system'];
    }
    else {
        $system = "Off";  //This is the default value to be selected when page first opened
    }

    if (isset($_POST['fan'])) {
        $fan = $_POST['fan'];
    }
    else {
        $fan = "Auto";  //This is the default value to be selected when page first opened
    }

    if (isset($_POST['newTemp'])) {
        $newTemp = $_POST['newTemp'];
    }

    if (isset($_POST['override'])) {
        $override = $_POST['override'];
    }
    else {
    $override = "N";
    }
}

?>

<script>

function cleanSubmit() {

    mainForm.elements['submitted'].value = false;
    mainForm.submit();
}

</script>

<p>
    <form id="mainForm" action="tstat_SetThermostat.php" method="post">
    Select Thermostat<br/>
<?php
    $j = 0;
    foreach ($thermostats as $namerow) {

        $checked = '';
        if ($j == $tsindex) {$checked = ' checked="checked"';}
        echo '<input type="radio" onclick = "cleanSubmit()" name="tsindex" value="' . $j . '"' . $checked . '> ' . $namerow[1] . ' </input><br/>';
        $j+=1;
    }
?>
    <br>
    System Control<br/>
    <input type="radio" name="system" value="Off" <?php if ($system == "Off") echo "checked='checked'"; ?> >OFF <br/>
    <input type="radio" name="system" value="Cool" <?php if ($system == "Cool") echo "checked='checked'"; ?> >COOL <br/>
    <input type="radio" name="system" value="Heat" <?php if ($system == "Heat") echo "checked='checked'"; ?> >HEAT <br/>
    <input type="radio" name="system" value="AuxHeat" <?php if ($system == "AuxHeat") echo "checked='checked'"; ?> >AUXILIARY HEAT <br/>
    <input type="radio" name="system" value="EmHeat" <?php if ($system == "EmHeat") echo "checked='checked'"; ?> >EMERGENCY HEAT <br/>
<!--    <input type = "submit" name="Off" value="OFF" style="width:170px; height:40px"/input>
    <input type = "submit" name="Cool" value="COOL" style="width:170px; height:40px"/input>
    <input type = "submit" name="Heat" value="HEAT" style="width:170px; height:40px"/input>
    <input type = "submit" name="EmerHeat" value="EMERGENCY HEAT" style="width:170px; height:40px"/input>
-->    <br>
    Fan Control<br/>
    <input type="radio" name="fan" value="On" <?php if ($fan == "On") echo "checked='checked'"; ?> >ON <br/>
    <input type="radio" name="fan" value="Auto" <?php if ($fan == "Auto") echo "checked='checked'"; ?> >AUTO <br/>
<!--    <input type = "submit" name="FanOff" value="OFF" style="width:170px; height:40px"/input>
    <input type = "submit" name="FanAuto" value="AUTO" style="width:170px; height:40px"/input>
-->    <br>
    Program Override<br/>
    Select Temperature<br/>
<!--    <input type="text" name="newTemp" size="10" value="<?php $newTemp/10;?>"</input> &nbsp;&nbsp;&nbsp;&nbsp;&nbsp; -->
<!--    <?php echo "<input type='text' name='newTemp' size='10' value='$newTemp'</input> &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"; ?> -->
    <input type='text' name='newTemp' size='10' <?php if (isset($newTemp)) echo "value='" . $newTemp . "'"; ?> ></input> &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
<!--    <input type = "submit" name="Temp" value="TEMPORARY" style="width:170px; height:40px"/input>
    <input type = "submit" name="Hold" value="HOLD" style="width:170px; height:40px"/input>
-->
    <input type="radio" name="override" value="T" <?php if ($override=="T") echo "checked='checked'"; ?>>TEMPORARY &nbsp;&nbsp;
    <input type="radio" name="override" value="H" <?php if ($override=="H") echo "checked='checked'";  ?>>HOLD &nbsp;&nbsp;
    <input type="radio" name="override" value="N" <?php if ($override=="N") echo "checked='checked'";  ?>>RESUME PROGRAM <br/>
    <br></br>

    <input type = "submit" name="Submit" value="Submit" style="width:100px; height:30px"/input>

</p>

    <p><input type="hidden" name="submitted" value="TRUE" /></p>
    <br>
<!--    <input type="radio" name="duration" value="Indef" <?php if ((isset($_POST['duration'])) && ($_POST['duration'] == "Indef")) echo 'checked="checked"'; ?> </input>Indefinitely<br></br>
    <input type="radio" name="duration" value="For" <?php if ((!isset($_POST['duration'])) || ($_POST['duration'] == "For")) echo 'checked="checked"'; ?></input>For &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
    <input type="text" name="howLong" size="10" value="<?php if (isset($_POST['howLong'])) echo $_POST['howLong']; else echo '0:15'; ?>"</input><br></br>
    <input type="radio" name="duration" value="Until" <?php if ((isset($_POST['duration'])) && ($_POST['duration'] == "Until")) echo 'checked="checked"'; ?> </input>Until  &nbsp;&nbsp;
    <input type="text" name="toDate" size="10" value="<?php if (isset($_POST['toDate'])) echo $_POST['toDate']; else echo date('n/j/y'); ?>"</input> &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
    <input type="text" name="toTime" size="10" value="<?php if (isset($_POST['toTime'])) echo $_POST['toTime']; else echo date('g:i A',strtotime("+1 hour", time())); ?>"</input><br></br>
    <input type="radio" name="duration" value="Timer" <?php if ((isset($_POST['duration'])) && ($_POST['duration'] == "Timer")) echo 'checked="checked"'; ?> </input>Revert to Timer<br></br>
-->
</form>
<body>


<?php  //Code to send the desired instructions to the thermostat

if (isset($_POST['submitted']) && $_POST['submitted']=="TRUE") {
    //Create the instruction the server will recognize
    $instruction = "!TST/" . $CommPwd . "/" . $tsnode . "/";
    switch ($system):
        case "Off":
            $instruction = $instruction . "0,";
            break;
        case "Cool":
            $instruction = $instruction . "A,h=0,";
            break;
        case "Heat":
            $instruction = $instruction . "A,h=1,";
            break;
        case "AuxHeat":
            $instruction = $instruction . "A,h=3,";
            break;
        case "EmHeat":
            $instruction = $instruction . "A,h=2,";
            break;
    endswitch;

    if ($fan == "On") $instruction = $instruction . "f=1,";
    else $instruction = $instruction . "f=0,";

    if (isset($newTemp) && ($system != "Off")) {
        $instruction = $instruction . "s=$newTemp$override,w#\n";
    }
    else {
        $instruction = $instruction . "s=$override,w#\n";
    }


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
 ?>


</body>
</html>
