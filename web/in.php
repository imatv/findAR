<?php

// findAR web endpoint

echo "var dump:";
var_dump($_GET);

$cmd = rawurldecode($_GET["cmd"]);
file_put_contents('data.txt', $cmd);

?>
