<!DOCTYPE html>
<html>
<head>
<link rel="stylesheet" href="style.css">
<meta charset="utf-8"/>
</head>
<script type="text/javascript">
function startTime()
{
	var today=new Date();
	var h=today.getHours();
	var m=today.getMinutes();
	var s=today.getSeconds();
	// add a zero in front of numbers<10
	m=checkTime(m);
	s=checkTime(s);
	document.getElementById('txt').innerHTML=h+":"+m+":"+s;
	t=setTimeout('startTime()',500);
}
function checkTime(i)
{
	if (i<10)
	{
		i="0" + i;
	}
	return i;
}
</script>
<script src="https://cdn.socket.io/socket.io-1.3.5.js"></script>
<script src="https://ajax.googleapis.com/ajax/libs/jquery/2.1.3/jquery.min.js"></script>
<script type="text/javascript">
var socket = io.connect('127.0.0.1:1883');
  socket.on('connect', function () {
    console.log("connected");
    socket.on('mqtt', function (msg) {
      if (msg.topic == "hello_topic") {
	  document.getElementById('messages').innerHTML=msg.payload;
	}   
    });
    socket.emit('subscribe',{topic:'hello_topic'});
  });
</script>
<body onload="startTime()">
<div class="time" id="txt"></div>
<div class="mqtt" id="messages"></div>
</body>
</html>
