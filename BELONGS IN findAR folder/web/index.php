<?php

// findAR web endpoint

// get app data
$data = trim(file_get_contents('data.txt'));
echo $data;
file_put_contents('data.txt','null');

?>
