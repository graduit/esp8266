<?php
/* This is a simple PHP example to host your own Amazon Alexa Skill written in PHP.
In my case it connects to my ESP8266/Arduino with two intents;
1: Turn on LED
2: Turn off LED
This Script contains neccessary calls and security to give you an easy to use DIY example.
*/

// Forces disabling of caching
header('Cache-Control: no-cache, must-revalidate'); // HTTP/1.1
header('Expires: Mon, 26 Jul 1997 05:00:00 GMT'); // Date in the past

// setupArray / CONFIG
$setupArray = array(
	'skillName' => "IotLedOn",
	'skillVersion' => '1.0',
	// TODO: Set the application id (Currently for my ESP8266 example)
	'applicationId' => 'amzn1.ask.skill.yourskillidhere', // From your Alexa developer console
	'checkSignatureChain' => true, // Make sure the request is a true amazonaws api call
	'reqValidTime' => 60, // Time in Seconds a request is valid
	// TODO: Set the account id
	'awsAccount' => 'amzn1.ask.account.youraccountidhere', // If this is != empty the specified session->user->userId is required. This is useful for account bound private only skills
	'validIp' => false,
	// 'validIp' => array(
	// 	"72.21.217.", // E.g. for me it was 72.21.217.66 from Amazon, when testing
	// 	"54.240.197."
	// ), // Limit allowed requests to specified IPv4, set to FALSE to disable the check.
	'lcTime' => "en_NZ" // "en_US"
	// We use NZ Echo so we want our date output to be N.Z.
);
setlocale(LC_TIME, $setupArray['lcTime']);

// Getting Input - Captures Amazon's POST JSON request
$rawJson = file_get_contents('php://input');
$echoRequestObject = json_decode($rawJson);
if (is_object($echoRequestObject) === false) {
	ThrowRequestError();
}
$requestType = $echoRequestObject->request->type;

// -----------------------------------------------------------------------------------------//
//					    Validate the request
// -----------------------------------------------------------------------------------------//

// Log the IP Address from Amazon
$file = 'test.txt';
// Open the file to get existing content
$current = file_get_contents($file);
// Append a new person to the file
$current .= $_SERVER['REMOTE_ADDR'] . "\n";
// Write the contents back to the file
file_put_contents($file, $current);

// Check if Amazon is the Origin
if (is_array($setupArray['validIp'])) {
	$isAllowedHost = false;
	foreach ($setupArray['validIp'] as $ip) {
		if (stristr($_SERVER['REMOTE_ADDR'], $ip)) {
			$isAllowedHost = true;
			break;
		}
	}
	if ($isAllowedHost == false) {
		ThrowRequestError(403, "Forbidden, your Host is not allowed to make this request!");
	}
	unset($isAllowedHost);
}

// Check if correct requestId
if (empty($echoRequestObject->session->application->applicationId)
	|| strtolower($echoRequestObject->session->application->applicationId) != strtolower($setupArray['applicationId'])) {
	ThrowRequestError(401, "Forbidden, unknown Application ID!");
	// TODO: Note, the below line - would need to handle for... but I think was mainly for audioplayer type tasks..
	// Some requests come in without a session - and in that case the ApplicationID is at $echoReqObj->context->system->application->applicationId
}
// Check SSL Signature Chain
if ($setupArray['checkSignatureChain'] == true) {
	if (preg_match("/https:\/\/s3.amazonaws.com(\:443)?\/echo.api\/*/i", $_SERVER['HTTP_SIGNATURECERTCHAINURL']) == false) {
		ThrowRequestError(403, "Forbidden, unknown SSL Chain Origin!");
	}
	// PEM Certificate signing check
	// First we try to cache the pem file locally
	$localPemHashFile = sys_get_temp_dir() . '/' . hash("sha256", $_SERVER['HTTP_SIGNATURECERTCHAINURL']) . ".pem";
	if (!file_exists($localPemHashFile)) {
		file_put_contents($localPemHashFile, file_get_contents($_SERVER['HTTP_SIGNATURECERTCHAINURL']));
	}
	$localPem = file_get_contents($localPemHashFile);
	if (openssl_verify($rawJson, base64_decode($_SERVER['HTTP_SIGNATURE']) , $localPem) !== 1) {
		ThrowRequestError(403, "Forbidden, failed to verify SSL Signature!");
	}
	// Parse the Certificate for additional Checks
	$cert = openssl_x509_parse($localPem);
	if (empty($cert)) {
		ThrowRequestError(424, "Certificate parsing failed!");
	}
	// SANs Check
	if (stristr($cert['extensions']['subjectAltName'], 'echo-api.amazon.com') != true) {
		ThrowRequestError(403, "Forbidden! Certificate SANs Check failed!");
	}
	// Check Certificate Valid Time
	if ($cert['validTo_time_t'] < time()) {
		ThrowRequestError(403, "Forbidden! Certificate no longer Valid!");
		// Deleting locally cached file to fetch a new at next req
		if (file_exists($localPemHashFile)) {
			unlink($localPemHashFile);
		}
	}
	// Cleanup
	unset($localPemHashFile, $cert, $localPem);
}
// Check Valid Time
if (time() - strtotime($echoRequestObject->request->timestamp) > $setupArray['reqValidTime']) {
	ThrowRequestError(408, "Request Timeout! Request timestamp is too old.");
}
// Check AWS Account bound, if this is set only a specific aws account can run the skill
if (!empty($setupArray['awsAccount'])) {
	if (empty($echoRequestObject->session->user->userId)
		|| $echoRequestObject->session->user->userId != $setupArray['awsAccount']) {
		ThrowRequestError(403, "Forbidden! Access is limited to one configured AWS Account.");
	}
}
// The JSON response to send back from the Alexa API call
$jsonOut = getJsonMessageResponse($requestType, $echoRequestObject);
header('Content-Type: application/json');
header('Content-length: ' . strlen($jsonOut));
echo $jsonOut;
exit();

// -----------------------------------------------------------------------------------------//
//					     Functions
// -----------------------------------------------------------------------------------------//
// This function returns a json blob for output
function getJsonMessageResponse($requestMessageType, $echoRequestObject) {
	GLOBAL $setupArray;
	$requestId = $echoRequestObject->request->requestId;
	$returnValue = "";
	if ($requestMessageType == "LaunchRequest")	{
		$returnDefaults = array(
			'version' => $setupArray['skillVersion'],
			'sessionAttributes' => array(
				// 'countActionList' => array(
				// 	'read' => true,
				// 	'category' => true
				// )
			),
			'response' => array(
				'outputSpeech' => array(
					// 'type' => "PlainText",
					// 'text' => "Welcome to the Basic Skill Example"
					'type' => "SSML",
					'ssml' => "<speak>Welcome to the e s p eight two six six example</speak>"
				),
				'card' => array(
					'type' => "Simple",
					'title' => "ESP8266 LED Control",
					'content' => "Test Card Content"
				),
				'reprompt' => array(
					'outputSpeech' => array(
						// 'type' => "PlainText",
						// 'text' => "Can I help you further?"
						"type" => "SSML",
						"ssml" => "<speak>Can I help you further?</speak>"
					)
				)
			),
			'shouldEndSession' => true
		);
		$returnValue = json_encode($returnDefaults);
	} else if ($requestMessageType == "SessionEndedRequest") {
		$returnValue = json_encode(array(
			'type' => "SessionEndedRequest",
			'requestId' => $requestId,
			'timestamp' => date("c"),
			'reason' => "USER_INITIATED"
		));
	} elseif ($requestMessageType == "IntentRequest") {
		// Alexa Intent name
		if ($echoRequestObject->request->intent->name == "LedOnIntent") { // Turns on LED
			// Do whatever your intent should do here.
			// In my case I do a post to my esp8266, getting a response, see function comment for more info.
			getRequestPayload(array(
				// Just for a quick test, sets the data up to match my ArduinoEsp8266-Template code
				'pin13' => 1,
			));
			$speakPhrase = "OK";
		} elseif ($echoRequestObject->request->intent->name == "LedOffIntent") { // Turns off LED
			getRequestPayload(array(
				'pin13' => 0, 
			));
			$speakPhrase = "OK";
		}
		$returnValue = json_encode(array(
			'version' => $setupArray['skillVersion'],
			'sessionAttributes' => array(
				'countActionList' => array(
					'read' => true,
					'category' => true
				)
			),
			'response' => array(
				'outputSpeech' => array(
					'type' => "PlainText",
					'text' => $speakPhrase
				),
				'card' => array(
					'type' => "Simple",
					'title' => "ESP8266 LED Control",
					'content' => $speakPhrase
				)
			),
			'shouldEndSession' => true
		));
	} else {
		ThrowRequestError();
	}
	return $returnValue;
} // end function getJsonMessageResponse


function ThrowRequestError($code = 400, $msg = 'Bad Request') {
	GLOBAL $setupArray;
	// Note there was an error. The error return code for all the checks (IP, Cert, etc) must return with status code 400.
	$code = 400; // This is the lazy/quick fix
	http_response_code($code);
	echo "Error " . $code . "<br />\n" . $msg;
	error_log("alexa/" . $setupArray['skillName'] . ":\t" . $msg, 0);
	exit();
}


/* This is just a custom function to get my connection to my home device.
	In this example it's a ESP8266 Led Control listening to POST requests (via Port Forwarding).
	It is using a let's encrypt SSL cert and a basic HTTP Auth.
	Use it as an example to DIY here: */
function getRequestPayload($payload) {
	
	// Uncomment for testing purposes
	// if (!empty($payload['action']) && $payload['action'] == 'toggleLed') {
		// return false;
	// }
	// return true;

	// Come back and sort this out..
	$username = "basicauthuser";
	$password = "basicauthpasswd";
	$host = "http://yourpublicipaddresshere"; // Enter your domain name, or IP Address here
	$process = curl_init($host);
//	curl_setopt($process, CURLOPT_SSL_VERIFYHOST, 2);
//	curl_setopt($process, CURLOPT_SSL_VERIFYPEER, 1);
//	curl_setopt($process, CURLOPT_USE_SSL, CURLUSESSL_ALL);
// 	curl_setopt($process, CURLOPT_SSL_VERIFYSTATUS, 1);
//	curl_setopt($process, CURLOPT_HEADER, FALSE); // Was giving me warnings..
//	curl_setopt($process, CURLOPT_USERPWD, $username . ":" . $password);
	curl_setopt($process, CURLOPT_TIMEOUT, 8);
	curl_setopt($process, CURLOPT_POST, 1);
	curl_setopt($process, CURLOPT_VERBOSE, FALSE);
	curl_setopt($process, CURLOPT_POSTFIELDS, http_build_query($payload));
	curl_setopt($process, CURLOPT_RETURNTRANSFER, TRUE);
	$return = curl_exec($process);
	if ($return == false) {
		throw new Exception(curl_error($process), curl_errno($process));
	}
	
//	curl_close($process);
	return $return;
	
}