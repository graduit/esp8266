-- NOTE this is a rough example of how to include jquery in the served up page, although usually you wouldn't do it like this..
-- Effectively works via e.g. 192.168.1.19/write/4/0 or 192.168.1.19/write/4/1 (for off/on respectively..)

-- Print the ESP8266 IP Address
print(wifi.sta.getip())

pin1=0
pin2=4
gpio.mode(pin1,gpio.OUTPUT)
gpio.mode(pin2,gpio.OUTPUT)
gpio.write(pin1, gpio.HIGH); -- Set off initially
gpio.write(pin2, gpio.HIGH);

function split(s, delimiter)
    result = {};
    for match in (s..delimiter):gmatch("(.-)"..delimiter) do
        table.insert(result, match);
    end
    return result;
end
function urlencode(payload)
    result = {};
    list=split(payload,"\r\n")
    --print(list[1])
    list=split(list[1]," ")
    --print(list[2])
    list=split(list[2],"\/")

    table.insert(result, list[1]);
    table.insert(result, list[2]);
    table.insert(result, list[3]);

    return result;
end

function sendHeader(conn)
     conn:send("HTTP/1.1 200 OK\r\n")
     conn:send("Access-Control-Allow-Origin: *\r\n")
     conn:send("Content-Type: application/json; charset=utf-8\r\n")
     conn:send("Server:NodeMCU\r\n")
     conn:send("Connection: close\r\n\r\n")
end

function index(conn)
    local buf = "";
    buf = buf..'HTTP/1.1 200 OK\r\nConnection: keep-alive\r\nCache-Control: private, no-store\r\n\r\n';
    buf = buf..'<!DOCTYPE HTML><html><head><style>input[type=button] {font-size:large;width:10em;height:5em;}</style>';
    buf = buf..'<meta content="text/html;charset=utf-8"><title>ESP8266</title>';
    buf = buf..'<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0">';
    buf = buf..'<script src="https://ajax.googleapis.com/ajax/libs/jquery/1.11.2/jquery.min.js"></script>';
    buf = buf..'<script>$(document).ready(function(){';
    buf = buf..'$("#btn-4-on").click(function(){$.ajax({url:"/write/4/0",success:function(result){console.log(result)}});});';
    buf = buf..'$("#btn-4-off").click(function(){$.ajax({url:"/write/4/1",success:function(result){console.log(result);}});});';
    buf = buf..'$("#btn-0-on").click(function(){$.ajax({url:"/write/0/0",success:function(result){console.log(result);}});});';
    buf = buf..'$("#btn-0-off").click(function(){$.ajax({url:"/write/0/1",success:function(result){console.log(result);}});});';
    buf = buf..'});</script>';
    buf = buf..'<body bgcolor="#ffe4c4">';
    buf = buf..'<h2>GPIO0</h2><hr><div><input id="btn-0-on" type="button" value="ON" /> <input id="btn-0-off" type="button" value="OFF" /></div>';
    buf = buf..'<h2>GPIO4</h2><hr><div><input id="btn-4-on" type="button" value="ON" /> <input id="btn-4-off" type="button" value="OFF" /></div>';
    buf = buf..'</body></html>';

    -- Using multiline literal with [[ and ]], didn't seem to work..
    conn:send(buf);
end
srv=net.createServer(net.TCP)
srv:listen(80,function(conn)
    conn:on("receive",function(conn,payload)
      print("Http Request..")
      list=urlencode(payload) 
      if (list[2]=="") then 
          index(conn)
      elseif (list[2]=="write") then
        local pin = tonumber(list[3]) 
        local status = tonumber(list[4]) 
        gpio.write(pin, status) 
        sendHeader(conn)  
        conn:send("{\"result\":\"ok\",\"digitalPin\": "..pin..", \"status\": "..gpio.read(pin).."}")
      elseif (list[2]=="read") then 
        sendHeader(conn)  
        conn:send("{\"result\":\"ok\", \"digitalPins\": [{\"digitalPin\": "..pin1..", \"status\": "..gpio.read(pin1).."},{\"digitalPin\": "..pin2..", \"status\": "..gpio.read(pin2).."}]}")
      else 
        sendHeader(conn)  
        conn:send("{\"result\":\"error\",\"message\": \"command not found\"}")        
      end
      -- conn:close()
    end)
    conn:on("sent", function(client)
    client:close();
  end)
end)
