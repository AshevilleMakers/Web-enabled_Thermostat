<?php # Main index page for Home Energy Manager
session_start();

if (!isset($_SESSION['userID']))
{
    header('Location: http://' . $_SERVER['HTTP_HOST'] . '/tstat_login.php');
    exit;
}

$userID = $_SESSION['userID'];
$userName = $_SESSION['username'];
$page_title = 'Welcome!';
include ($_SERVER['DOCUMENT_ROOT'] . 'includes/tstat_MainHeader.php');

?>

</html>
