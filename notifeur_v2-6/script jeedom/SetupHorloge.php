<?php
   	$IP=$argv[1];
	$commande=urlencode(utf8_decode($argv[2]));
  	$etat=urlencode(utf8_decode($argv[3]));

$url = 'http://'.$IP;
if ($commande =="INT") $data = array("LUM" => "manu" , $commande => $etat);
else $data = array($commande => $etat);
// use key 'http' even if you send the request to https://...
$options = array(
  'http' => array(
    'header'  => "Content-type: application/x-www-form-urlencoded\r\n",
    'method'  => 'POST',
    'content' => http_build_query($data),
  ),
);
$context  = stream_context_create($options);
$result = file_get_contents($url, false, $context);
//var_dump($result);
?>