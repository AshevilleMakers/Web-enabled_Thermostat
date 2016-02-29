<?php #Connect to the load_controller database

DEFINE('DB_USER', 'root');  //Update with your MySQL username if different
DEFINE('DB_PASSWORD', 'password');  //Update with your MySQL password
DEFINE('DB_HOST', 'localhost');

DEFINE('DB_PORT', 3306);
DEFINE('DB_NAME', 'tstat_database');

//Connect to MySQL
$dbc = mysqli_connect(DB_HOST, DB_USER, DB_PASSWORD, DB_NAME, DB_PORT);
if (mysqli_connect_errno()) {
    echo "<p>Connect failed: " . mysqli_connect_error() . "<p/>";
    exit();
}

?>
