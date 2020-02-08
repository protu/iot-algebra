// Client for testing server.
var PORT = <port>;
var HOST = '127.0.0.1';


var dgram = require('dgram');

var message = new Buffer('1;35;91;17.56432;27.43215;27/01/2020 00:19:12;75');

 

var client = dgram.createSocket('udp4');

client.send(message, 0, message.length, PORT, HOST, function(err, bytes) {

    if (err) throw err;

    console.log('UDP client message sent to ' + HOST +':'+ PORT);

   

});

client.on('message', function (message, remote) {

    console.log(remote.address + ':' + remote.port +' - ' + message);

       client.close();

 

});