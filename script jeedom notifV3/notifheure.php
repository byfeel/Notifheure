<?php
  // maj 25-11-2018 
  // Script pour notif'heure ( validé pour version  V3.2 ) 
  if (isset($argv)) {
  	$IP=$argv[1];
	$MOO=$argv[2];
	 // debuggage
 //include('/var/www/html/core/class/scenario.class.php');
	
//	log::add('script','error','Argument 3 :'.urlencode(utf8_encode($argv[2])) ); 

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
      $notif=urlencode($argv[2]);

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
    $imp=$result["important"];
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
$url="http://".$IP."/Notification?msg=".$notif.$argtype."&intnotif=".$lum."&flash=".$flash."&txt=".$txt."&important=".$imp;
//log::add('script','error','url :'.$url); 
$httpfile  = file_get_contents($url);
    } 
  }
?>