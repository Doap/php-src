--TEST--
Phar object: basics
--SKIPIF--
<?php
if (!extension_loaded("phar")) die("skip");
if (version_compare(PHP_VERSION, "6.0", ">")) die("skip pre-unicode version of PHP required");
if (!extension_loaded("spl")) die("skip SPL not available");
?>
--INI--
phar.require_hash=0
phar.readonly=0
--FILE--
<?php

require_once 'files/phar_oo_test.inc';

$phar = new Phar($fname);
var_dump($phar->getVersion());
var_dump(count($phar));

class MyPhar extends Phar
{
	function __construct()
	{
	}
}

try
{
	$phar = new MyPhar();
	var_dump($phar->getVersion());
}
catch (LogicException $e)
{
	var_dump($e->getMessage());
}
try {
	$phar = new Phar('test.phar');
	$phar->__construct('oops');
} catch (LogicException $e)
{
	var_dump($e->getMessage());
}

?>
===DONE===
--CLEAN--
<?php 
unlink(dirname(__FILE__) . '/files/phar_oo_test.phar.php');
__halt_compiler();
?>
--EXPECT--
string(5) "1.0.0"
int(5)
string(103) "In the constructor of MyPhar, parent::__construct() must be called and its exceptions cannot be cleared"
string(29) "Cannot call constructor twice"
===DONE===
