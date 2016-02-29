<?php // Welcome and log-in page

$page_title = "Home Thermostat Login";
$name = $pwd = $out = "";
$salt1 = "&7tH$#";
$salt2 = "(oJ@";
session_start();

if (isset($_POST['name']) && isset($_POST['pwd']))
{
    require_once(dirname(dirname(__FILE__)) . '/private/tstat_ConnDb.php');
    $name = mysqli_fix_string($dbc, $_POST['name']);
    $pwd = mysqli_fix_string($dbc, $_POST['pwd']);

    //Check the username and password entered
    $query = "SELECT username, password, id from users WHERE username = '" . $name . "'";
    if ($result = mysqli_query($dbc, $query))
    {
        if (mysqli_num_rows($result) == 0)
        {
            $out = "Username/password combination of $name/$pwd not recognized.";
        }
        else
        {
            //Get the values
            while ($row = mysqli_fetch_assoc($result))
            {
                $dbPwd = $row['password'];
                $dbUsername = $row['username'];
                $dbUserID = $row['id'];
            }

            $token = md5("$salt1$pwd$salt2");
            if (($token == $dbPwd) && ($name == $dbUsername))
            {
                //Get started
                session_start();
                $_SESSION['username'] = $dbUsername;
                $_SESSION['userID'] = $dbUserID;
                header('Location: http://' . $_SERVER['HTTP_HOST'] . '/tstat_index.php');
		exit;
            }
            else
            {
                $out = "Username/password combination of $name/$pwd not recognized.";
            }
        }
    }
    else
    {
        $out = "Username/password combination of $name/$pwd not recognized.";
    }
}

echo <<<_END

<!DOCTYPE html>
<html>
<head>
    <base href="."></base>
<!--    <title><?php echo $page_title ?></title>  -->
    <title>$page_title</title>
    <h1 style="background-color:blue; font-family:timesnewroman;color:white;font-size:28px;"><b>Thermostat - Login</b></h1>
</head>

<body>
    <p>Please log in below.
    <br><br>

    <form method="post" action="tstat_login.php">
        Username  <input type="text" name = "name" size = "11" autofocus/>
        Password  <input type="password" name = "pwd" size = "11" />
                  <input type="submit" value="Log in" />
    </form>

    <br><b>$out</b>

</body>
</html>
_END;



function mysqli_fix_string($link, $string)
{
    if (get_magic_quotes_gpc()) $string = stripslashes($string);
    return mysqli_real_escape_string($link, $string);
}

?>
