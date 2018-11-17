<?php
  // maj 17-11-2018
  if (isset($argv)) {
  	$IP=$argv[1];
	$MOO=$argv[2];
	 // debuggage
/*	 include('/var/www/html/core/class/scenario.class.php');
	log::add('script','error','Argument 2 :'.$MOO); 
	log::add('script','error','Argument 3 :'.urlencode(utf8_encode($argv[2])) ); 
*/
	if ($MOO=="HOR" || $MOO=="SEC" ||  $MOO=="LUM" ||  $MOO=="INT" || $MOO=="NIG" || $MOO=="LED")
    {
     // Options 
      		$commande=urlencode(utf8_decode($argv[2]));
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
      $notif=iconv("UTF-8", "CP1252",$argv[2]);  // conversion utf8 en ISO 8859-15 - gestion de €
      $notif=urlencode($notif);

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
    if ( $type == "INFO") { 
      $pause=$result["pause"];         
      $argtype="&type=".$type."&pause=".$pause;              
        } else {
        $argtype="&type=".$type;
      }

/* Lit un fichier distant sur le serveur www.example.com avec le protocole HTTP */
$url="http://".$IP."/Notification?msg=".$notif.$argtype."&intnotif=".$lum."&flash=".$flash."&txt=".$txt;
$httpfile  = file_get_contents($url);
    } 
  }
?>