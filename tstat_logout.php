<?php  //logout.php
    session_start();

    $page_title = 'Logout';
    include ('./includes/tstat_MainHeader.php');

    $_SESSION = array();
    if (session_id() != "" || isset($_COOKIE[session_name()])) setcookie(session_name(), '', time() - 2592000, '/');
    session_destroy();
    echo "<p>You have successfully logged out.</p>";
    echo "<p><a href=tstat_index.php>Please click here to log back in.</p>"
?>
