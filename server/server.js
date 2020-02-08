const PORT = <port>;
const HOST = <host>
const dgram = require('dgram');
const server = dgram.createSocket('udp4');
const fs = require('fs');
const { Pool } = require('pg');
const uuid = require('uuid');

// Starting server.
server.on('listening', function() {
 const address = server.address();
 console.log('UDP Server listening on ' + address.address + ':' + address.port);
});

// Receiving message.
server.on('message', function(message, remote) {
 console.log(remote.address + ':' + remote.port +' - ' + message);
 log(` From : ${remote.address}:${remote.port}, Message: ${message} \n`);
 messageSplit = message.toString().split(';');
   var id = messageSplit[0];
   var temp = messageSplit[1];
   var lng = messageSplit[2];
   var speed = messageSplit[3]*100;
   var lat = messageSplit[4];
   var timeTotalString = messageSplit[5];
   var timeTotalStringSplit =timeTotalString.split(' ');
   var time = timeTotalStringSplit[1]+" "+ timeTotalStringSplit[0];
   var hum = messageSplit[6];

const pool = new Pool({
    host: 'localhost',
    user: 'postgres',
    max: 20,
    database: 'postgres',
    password: <password>,
    port: 5432,
    idleTimeoutMillis: 30000,
    connectionTimeoutMillis: 2000,
})

pool.connect((err, client, release) => {
  if (err) {
	  // Could not connect to postgres.
        log(`Error acquiring client: ${err} \n`);
        return console.error('Error acquiring client', err.stack)
    }
    client.query('INSERT INTO device (temperature, speed, latitude, longitude, $
        if (err) {
			// Could not execute query aka write to postgres.
            log(`Error executing query: ${err} \n`);
            return console.error('Error executing query', err.stack)
        }
		   // Rows added to postgres.
		  console.log(result.rows)
                log(`Rows added to postgres \n`);
    })
})

var okBuffer = new Buffer("OK " + message);
server.send(okBuffer, 0, okBuffer.length, remote.port, remote.address, function$
   if (err) {
	   // Print out that we did not send a response to client.
         log(`Error sending ok to client: ${err} \n`);
         console.error("Error sending OK buffer to client", err);
 else {
     // Print out to the server's console  and text file that we've successfully sent the response to client.
         log(`OK sent to client \n`);
         console.log("OK sent to client");
     }
});
});

// Add new info to the log file.
function log(message) {
      // Get time for logging.
    const currentTime = new Date();
    const year = currentTime.getFullYear();
    const month = currentTime.getMonth() + 1;
    const date = currentTime.getDate();
    const hours = currentTime.getHours();
    const minutes = currentTime.getMinutes();
    const seconds = currentTime.getSeconds();
    const time = `Time: ${date}.${month}.${year} ${hours}:${minutes}:${seconds}, `;
    const logMessage = time + message;
     // Add new data to file.
    fs.appendFile('server_logger.txt', logMessage, function (err) {
        if (err) {
            throw err;
       }
    });
}

server.bind(PORT,HOST);