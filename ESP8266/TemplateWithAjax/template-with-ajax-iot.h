const char AJAX_PAGE_HTML[] PROGMEM = R"=====(
<!DOCTYPE html> 
<html> 
<head>
	<title>IOT Ajax Test</title>
	<!-- <link href='https://fonts.googleapis.com/css?family=Roboto:300' rel='stylesheet' type='text/css'> -->
	<!-- <link href='main.css' rel='stylesheet' type='text/css'> -->
	<!-- <link rel="apple-touch-icon" sizes="180x180" href="/apple-touch-icon-180x180.png"> -->
	<!-- <link rel="icon" type="image/png" sizes="144x144"  href="/favicon-144x144.png"> -->
	<!-- <link rel="icon" type="image/png" sizes="48x48" href="/favicon.ico"> -->
	<!-- <link rel="manifest" href="/manifest.json"> -->
	<!-- <meta name="theme-color" content="#00878f"> -->
	<meta name="viewport" content="width=device-width, minimum-scale=1.0, maximum-scale=1.0, initial-scale=1">
	<!-- <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=0"> -->
	<!-- <script src="WebSocket.js" type="text/javascript"></script> -->
	<style> 
	body {
		text-align: center;
		max-width: 400px;
		margin: 10px auto;
	}
	</style>
</head> 
<body>
<div id="dataVals">
	<h4>LED Status</h4> 
	<div id="ledStatus">Null</div>
</div> 

<div id="updateView"> 
	<h4>Update Response Rate</h4> 	
	<label for="fader">(in milliseconds)</label>
	<input type="range" style="width: 300px" min="200" max="5000" value="2000" id="fader" step="1" oninput="outputUpdate(value)">
	<output for="fader" id="slide">2000</output>
	<br> 
	<br>
	<button id="toggleLedBtn" onclick="toggleLed()">Toggle Led</button>
</div> 

<script> 
	
	function loadData() { 
		var xhttp = new XMLHttpRequest();
		xhttp.onreadystatechange = function() { 
			if (this.readyState == 4 && this.status == 200) { 
			console.log(this.responseText);
				var obj = JSON.parse(this.responseText); 
				document.getElementById("ledStatus").innerHTML = obj.data[0].dataValue; 
				// document.getElementById("ledStatus").innerHTML = obj.data[1].ledState; 
			} 
		}; 
		
		xhttp.open("GET", "/getdata", true);
		xhttp.send(); 
	} 
	
	// Sets the ajax call rate as 2 seconds by default
	var timedEvent = setInterval(function(){
		loadData();
	}, 2000); 
	
	// Updates the ajax call rate, and the display
	function outputUpdate(val) { 
		clearInterval(timedEvent);
		timedEvent = setInterval(function(){
			loadData();
		}, val); 
		document.querySelector("#slide").value = val; 
	}
	
	var randomStartingLedStatus = 0; // Random because it isn't synced with the response from the ESP8266 initially (Got a bit lazy/hastey.. not a big issue so yeah..)
	function toggleLed() {
		randomStartingLedStatus = !randomStartingLedStatus;
				
		var xhttp = new XMLHttpRequest();
		xhttp.onreadystatechange = function() { 
			if (this.readyState == 4 && this.status == 200) { 
				// console.log(this.responseText);
				var obj = JSON.parse(this.responseText); 
				// document.getElementById("...").innerHTML = obj.data[0].dataValue; 
				document.getElementById("ledStatus").innerHTML = obj.data[1].ledState; 
			} 
		}; 
		
		xhttp.open("POST", "/testajaxpost", true);	
		// Note: Without setting the header below, you send straight JSON data which the
		// ESP8266 library isn't equipped to handle? So it gets sent as e.g. server.argName = 'plain' and server.arg = 'ledstate=true' (or 'false')...		
		// xhttp.setRequestHeader('Content-type', 'application/x-www-form-urlencoded');
		xhttp.setRequestHeader("Content-Type", "application/json;charset=UTF-8");
		
		/* xhttp.onload = function () {
			// Do something to response
			console.log(this.responseText);
		}; */
				
		// var params = 'ledstate=' + randomStartingLedStatus /* + '&requiredkey=key' */;
		var params = JSON.stringify(
			{"data": [{"dataValue": "test"}, {"ledState": randomStartingLedStatus}],
			 "test": "anothervalue"}
		)
		xhttp.send(params);
	}
</script> 
</body> 
</html>
)=====";