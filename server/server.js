'use strict'; // this server is a placeholder until it's rewritten in C# due to the lack of good screenshot/recording libraries

const net = require("net"),
      //{ performance } = require("perf_hooks"),
	  screenshot = require("screenshot-desktop"), // this screenshot library is horrible but it's all i could find. an executable is ran each time which takes about 100ms on my system. see comment on setInterval
	  sharp = require("sharp"),
      footer = Buffer.from([0x55, 0x44, 0x55, 0x11]), // both jpeg footer and incoming frame request from client
      port = 5451,
	  resolution = { x: 240, y: 135 },
	  jpegQuality = 50, // 0-100. 50 gets about 30fps. 70 will average around 24.
	  screenCapInterval = 30; // see setInterval

var screenshotBuffer;

async function captureScreenshot() {
	var screen = await screenshot({format: "jpeg"});

	var img = await sharp(screen);
	await img.resize(resolution.x, resolution.y);
	await img.jpeg({
		quality: jpegQuality, 
		optimiseCoding: true,
		trellisQuantisation: true,
		overshootDeringing: false,
		chromaSubsampling: "4:2:0",
		progressive: false,
		mozjpeg: false,
		force: true
	});

	screenshotBuffer = await img.toBuffer();
}

setInterval(captureScreenshot, screenCapInterval); // fetch many consecutive screenshots as a workaround to the massive latency. you can increase the interval time but you'll get a choppier video 
  
const server = net.createServer((socket) => {
	console.log("Client connected.");
	
	socket.on("data", data => {
		if(data.length === 4 && data[0] == footer[0] && data[1] == footer[1] && data[2] == footer[2] && data[3] == footer[3]) { // client is requesting new frame
			//console.log("Frame requested");
			
			getImage().then(buffer => socket.write(buffer, undefined, function() {
				//console.log("Frame sent")
			}));
		}
	});
	
	getImage().then(buffer => socket.write(buffer, undefined, function() {
		//console.log("Frame sent")
	}));
	
	socket.on("end", () => console.log("Client disconnected"));
	
	socket.on("error", (err) => {
		if(err.code === "ECONNRESET") {
			console.log("Client connection reset");
		} else {
			console.error(err);
		}
	});
});

async function getImage() {
	var newBuffer = await Buffer.concat([screenshotBuffer, footer]); // append footer to signal end of data
	
	return newBuffer;
}

server.on("error", (err) => {
	throw err;
});

server.listen(port, () => {
  console.log("Server running");
});