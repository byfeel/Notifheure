<?php
  // maj 29-12-2018
  // Script pour notif'heure ( validé pour version  V3.2 )
  // Byfeel.info
  if (isset($argv)) {
  	$IP=$argv[1];
	$MOO=$argv[2];
	 // debug
 //include('/var/www/html/core/class/scenario.class.php');

//	log::add('script','error','Argument 3 :'.urlencode(utf8_encode($argv[2])) );
  //$argv[2]=html_entity_decode($argv[2]);

	if ($MOO=="HOR" || $MOO=="SEC" ||  $MOO=="LUM" ||  $MOO=="INT" || $MOO=="NIG" || $MOO=="LED")
    {
     // Options
      		$commande=urlencode(($argv[2]));
  			$etat=urlencode(utf8_decode($argv[3]));

			$url = 'http://'.$IP.'/Options';
			$data = array($commande => $etat);
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
    } else {
      // Notifications
  //log::add('script','error','message avant:'.$notif);
      $notif=rawurlencode(html_entity_decode($argv[2],ENT_QUOTES));
	//log::add('script','error','message :'.$notif);
	// Decoupage des options transmis dans titre sous la forme type = Animation ; lum = intensite de la lumiére  ; etc
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
    $flash=$result["flash"];
    $txt=$result["txt"];
   if (array_key_exists("important",$result)) $imp="&important=";
           else $imp="";
    if ( $type == "INFO") {
      $pause=$result["pause"];
      	$fi=$result["fi"];
    	$fo=$result["fo"];
    	$fio=$result["fio"];
      	if (!is_numeric($fi)) $fi=8;
    	if (!is_numeric($fo)) $fo=1;
      	if (is_numeric($fio)) {
          	$fi=$fio;
      		$fo=$fio;
        }
      	if (!is_numeric($pause)) $pause=3;
      	$argtype="&type=".$type."&pause=".$pause."&fi=".$fi."&fo=".$fo;
        } else {
        $argtype="&type=".$type;
      }



/* Lit un fichier distant sur le serveur www.example.com avec le protocole HTTP */
$url="http://".$IP."/Notification?msg=".$notif.$argtype."&intnotif=".$lum."&flash=".$flash."&txt=".$txt.$imp;
//log::add('script','error','url :'.$url);
$httpfile  = file_get_contents($url);
    }
  }
?>
