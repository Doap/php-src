--TEST--
filter_var() and FILTER_VALIDATE_REGEXP
--FILE--
<?php

var_dump(filter_var("data", FILTER_VALIDATE_REGEXP, array("options"=>array("regexp"=>'/.*/'))));
var_dump(filter_var("data", FILTER_VALIDATE_REGEXP, array("options"=>array("regexp"=>'/^b(.*)/'))));
var_dump(filter_var("data", FILTER_VALIDATE_REGEXP, array("options"=>array("regexp"=>'/^d(.*)/'))));
var_dump(filter_var("data", FILTER_VALIDATE_REGEXP, array("options"=>array("regexp"=>'/blah/'))));
var_dump(filter_var("data", FILTER_VALIDATE_REGEXP, array("options"=>array("regexp"=>'/\[/'))));
var_dump(filter_var("data", FILTER_VALIDATE_REGEXP));

echo "Done\n";
?>
--EXPECTF--	
string(4) "data"
NULL
string(4) "data"
NULL
NULL

Warning: filter_var(): 'regexp' option missing in %s on line %d
NULL
Done
