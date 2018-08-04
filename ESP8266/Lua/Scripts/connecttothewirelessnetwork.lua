-- Connect to the wireless network

-- NOTE: Comments take up memory as well... (this file seems to exceed the limit I have available so made this version for comments inclusive...)

-- Note the filename has to be all lowercase, no spaces, underscores, hyphens, or numbers.
-- "Save to ESP" the json.lua file, before this file.
-- "Send to ESP" works for limited content, i.e. you can get the below to work if you remove the
-- comments and excess code. As is, you would get a "string length overflow" error.
-- Turns out the default max string length in Lua for NodeMCU is 4096 bytes.
-- It can be changed with collectgarbage("setmemlimit", kilobytes) at runtime, however there is only about 40 kB usable SRAM.
-- Could look into an ESP32, which has 520 kB...
-- i.e.

-- Get current memlimit in Kbytes
-- memlimit = collectgarbage("getmemlimit");
-- print(memlimit)
-- Set memlimt to 64Kbytes
-- collectgarbage("setmemlimit", 64);

-- However, you can also store/send the entire file by using "Save to ESP" (instead of "Send to ESP")
-- NOTE: filename cannot be greater than 27 characters for ESPlorer...

-- Hint: Click "Reload" on the right panel, to print the memory and number of files stored on the device to the serial monitor

-- Note: This basically works by uploading this file (and json.lua) to the ESP8266, opening the supplementary "myesp8266control.html" file, and interacting with the controls on the webpage

-- Get the ESP8266 IP Address
print(wifi.sta.getip())

-- Your access point's SSID and password
SSID = "Your Wi-Fi Name"
SSID_PASSWORD = "Your Wi-Fi Password"
station_cfg={}
station_cfg.ssid="Your Wi-Fi Name"
station_cfg.pwd="Your Wi-Fi Password"
station_cfg.save=false

-- Configure ESP as a station (i.e. connects to some other WiFi AP)
wifi.setmode(wifi.STATION)
wifi.sta.config(station_cfg)
-- wifi.sta.autoconnect(1) -- Auto connects to AP in station mode.
print(wifi.sta.getip())
--192.168.1.19

led1 = 0 -- pin 0 and 4 = LEDs on the NodeMCU
led2 = 4
gpio.mode(led1, gpio.OUTPUT)
gpio.mode(led2, gpio.OUTPUT)
gpio.write(led1, gpio.HIGH); -- Set off initially
gpio.write(led2, gpio.HIGH);

--[[ For Arduino-like Input
pin = 1
gpio.mode(pin,gpio.INPUT)
print(gpio.read(pin))
]]

-- https://gist.github.com/tylerneylon/59f4bcf316be525b30ab
-- Note you must save/"Send to ESP" first, for it to work
-- Doesn't automatically know where it is just because it's in the same folder..
json = require("json")

-- A simple HTTP server
srv=net.createServer(net.TCP) -- https://nodemcu.readthedocs.io/en/master/en/modules/net/#returns
srv:listen(80,function(conn)
  conn:on("receive", function(client,request)

    -- print(request)	

--    tmr.alarm([id/ref], interval_ms, mode, func()
--    tmr.alarm(1,2000,tmr.ALARM_SINGLE,function()
--        print(request)
--    end)
  
    --[[
        when the page is accessed by a web browser, the "request" is set to e.g. 

        GET / HTTP/1.1
        Host: 192.168.1.18
        Connection: keep-alive
        Upgrade-Insecure-Requests: 1
        User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/67.0.3396.99 Safari/537.36
        Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8
        Accept-Encoding: gzip, deflate
        Accept-Language: en-NZ,en-GB;q=0.9,en-US;q=0.8,en;q=0.7
        
        
        GET /favicon.ico HTTP/1.1
        Host: 192.168.1.18
        Connection: keep-alive
        User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/67.0.3396.99 Safari/537.36
        Accept: image/webp,image/apng,image/*,*/*;q=0.8
        Referer: http://192.168.1.18/
        Accept-Encoding: gzip, deflate
        Accept-Language: en-NZ,en-GB;q=0.9,en-US;q=0.8,en;q=0.7

        
        After ?pin=OFF2
                
        GET /?pin=OFF2 HTTP/1.1
        Host: 192.168.1.18
        Connection: keep-alive
        Upgrade-Insecure-Requests: 1
        User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/67.0.3396.99 Safari/537.36
        Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8
        Referer: http://192.168.1.18/
        Accept-Encoding: gzip, deflate
        Accept-Language: en-NZ,en-GB;q=0.9,en-US;q=0.8,en;q=0.7
        
        
    ]]
    local buf = "";
    -- Note the underscore variables are simply throwaway variables (i.e. don't care about the value)
    -- In this case they would be the first and last index of the string being searched for..
    -- When a query is passed, method gets filled as e.g. GET, path as e.g. '/', and vars as e.g. pin=OFF2
    -- When nothing is passed, method, path and vars = nil (because group 2 comes back as undefined... I think..)
    -- First group is any length of capital chars..
    -- Second group matches any char except line breaks (match 1 or more of them) - ACTUALLY, in this case it's the query param separator
    -- question marks => match between - and 1 of the preceding token
    -- Third group is any char except line breaks (match 1 or more of them..)
    local _, _, method, path, vars = string.find(request, "([A-Z]+) (.+)?(.+) HTTP");
    -- Try again without the attempt to search for query params
    -- E.g. returns method as GET, and path as e.g. '/'
    if (method == nil) then
        _, _, method, path = string.find(request, "([A-Z]+) (.+) HTTP");
    end
    
    local _GET = {}
    if (vars ~= nil) then
      -- matches any words.. where the * signifies 0 or more of the preceding token (i.e. the &)
      for k, v in string.gmatch(vars, "(%w+)=(%w+)&*") do
        _GET[k] = v
      end
    end
	
	-- If you want to serve a page from the ESP8266, although this isn't recommended
    buf = buf.."<h1>ESP8266 Web Server</h1>";
    buf = buf.."<p>GPIO0 <a href=\"?pin=ON1\"><button>ON</button></a>&nbsp;<a href=\"?pin=OFF1\"><button>OFF</button></a></p>";
    buf = buf.."<p>GPIO2 <a href=\"?pin=ON2\"><button>ON</button></a>&nbsp;<a href=\"?pin=OFF2\"><button>OFF</button></a></p>";
    
    local _on,_off = "",""
    if (_GET.pin == "ON1") then
      gpio.write(led1, gpio.LOW); -- Set the pin HIGH (NodeMCU has reverse voltage..)
    elseif (_GET.pin == "OFF1") then
      gpio.write(led1, gpio.HIGH);
    elseif(_GET.pin == "ON2") then
      gpio.write(led2, gpio.LOW);
    elseif(_GET.pin == "OFF2") then
      gpio.write(led2, gpio.HIGH);
    end

	-- Some browsers (such as chrome) send xmlhttprequest POSTs as two packets
    -- One for the POST headers, and one for the POST body
    -- So you need the method == nil check
	-- (Note Microsoft Edge seems to strangely do four requests...)
    if (method == "POST" or method == nil) then
      -- Note: Hyphen is one of the regexp "magic" symbols. Try putting a % in front of it (if it's not working.
      -- POST body generally comes after two new lines..
      local _, _, dataJsonString = string.find(request, "\r\n\r\n(.+)");
      
	  -- Only body returned when 2 packets...
	  if (method == nil) then
        dataJsonString = request;
      end

	  if (dataJsonString ~= nil) then
		-- print(dataJsonString);
		dataJson = json.parse(dataJsonString)
		if (dataJson.pin == "ON1") then
		  gpio.write(led1, gpio.LOW); -- Set the pin HIGH (NodeMCU has reverse voltage..)
		elseif (dataJson.pin == "OFF1") then
		  gpio.write(led1, gpio.HIGH);
		elseif(dataJson.pin == "ON2") then
		  gpio.write(led2, gpio.LOW);
		elseif(dataJson.pin == "OFF2") then
		  gpio.write(led2, gpio.HIGH);
		end
	  end
    end
    
    -- For http CORS (i.e. with XMLHttpRequest)
    allowOrigin = '*' --'file://' -- or null (from chrome..)
    allowMethods = 'POST, GET'
    allowHeaders = '*'
    responseHeaders = 'HTTP/1.1 200 OK\r\n'
    -- contentType = 'text/html'
    contentType = 'application/json'
    responseHeaders = responseHeaders .. 'Access-Control-Allow-Origin: ' .. allowOrigin .. '\r\n'
    responseHeaders = responseHeaders .. 'Access-Control-Allow-Methods: ' .. allowMethods .. '\r\n'
    responseHeaders = responseHeaders .. 'Access-Control-Allow-Headers: ' .. allowHeaders .. '\r\n'
    responseHeaders = responseHeaders .. 'Content-type: ' .. contentType .. '\r\nServer: ESP8266-1\r\n\n'

    -- Preflight check
    if (method == "OPTIONS") then
        client:send(responseHeaders)
    end
    
    -- responseHeaders = "HTTP/1.1 200 OK\r\nServer: NodeMCU on ESP8266\r\nContent-Type: text/html\r\n\r\n"
    
    if (method == "GET" or method == "POST" or method == nil) then
        -- client:send(responseHeader .. buf);

        local _, _, dataJsonString = string.find(request, "\r\n\r\n(.+)");
        if (dataJsonString ~= nil) then     
            -- For returning Content-type text/html   
            -- client:send(responseHeaders .. "Test Response");

            -- For returning Content-type json
            dataJson = json.parse(dataJsonString)
            client:send(responseHeaders .. '{"pin":"' .. dataJson.pin .. '"}"');
        elseif (method == nil and request ~= nil) then
            -- For returning Content-type text/html   
            -- client:send(responseHeaders .. "Test Response");

            -- For returning Content-type json
            dataJsonString = request
            dataJson = json.parse(dataJsonString)
            client:send(responseHeaders .. '{"pin":"' .. dataJson.pin .. '"}"');
        end
    end
	-- client:close(); (old style.. i.e 0.9.x)
    -- collectgarbage();
  end)
  -- (new style..)
  conn:on("sent", function(client)
    client:close();
  end)
end)


-- https://nodemcu.readthedocs.io/en/master/en/modules/http/
-- HTTP GET example
-- http.get("http://httpbin.org/ip", nil, function(code, data) -- returns your public ip as ajax...
-- @params url
-- @params headers - Optional additional headers to append, including \r\n; may be nil
-- @params callback
-- @returns nil
--http.get("http://api.open-notify.org/astros.json", nil, function(code, data)
--  if (code < 0) then
--  else
--    print(code, data)
--    -- Where code is e.g. 200 for success
--    -- and data is e.g. the json data..
--  end
--end)
-- Note, these are processed as AJAX, so they may not show in console in the correct order...
-- will need to search for them in the serial monitor a bit..



-- HTTP POST example
-- @params url The URL to fetch, including the http:// or https:// prefix
-- @params headers Optional additional headers to append, including \r\n; may be nil
-- @params body The body to post; must already be encoded in the appropriate format, but may be empty
-- @params callback The callback function to be invoked when the response has been received or an error occurred; it is invoked with the arguments status_code, body and headers. In case of an error status_code is set to -1.
-- @returns nil
--http.post('http://httpbin.org/post',
--  'Content-Type: application/json\r\n',
--  '{"hello":"world"}',
--  function(code, data)
--    if (code < 0) then
--      print("HTTP request failed")
--    else
--      print("POST")
--      print(code, data)
--
--      dataJson = json.parse(data)
--      print(dataJson.data)
--    end
--  end)
