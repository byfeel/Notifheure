<?php
  	$IP=$argv[1];
	$notif=iconv("UTF-8", "CP1252", $argv[2]);  // conversion utf8 en ISO 8859-15 
	$notif=urlencode($notif);

// Decoupage des options transmis dans titre sous la forme type = Animation ; lum = intensite de la lumiére
	//$Options = explode(";",str_replace(' ', '', $argv[3]));
	$Options=preg_split("/[\,\;\.\:\-]/", str_replace(' ', '', $argv[3]));
	foreach ( $Options as $Value ){
    list($k, $v) = explode("=",$Value);
    $k = strtolower($k);
    $v = strtoupper($v);
    $result[ $k ] = $v;
    }
// Affectation des differents index
	$type=$result["type"];
	$lum=$result["lum"];

/* Lit un fichier distant sur le serveur www.example.com avec le protocole HTTP */
$url="http://".$IP."/Notification?msg=".$notif."&type=".$type."&intnotif=".$lum;
$httpfile  = file_get_contents($url);
?>