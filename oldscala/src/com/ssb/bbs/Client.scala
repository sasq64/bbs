package com.ssb.bbs

import java.nio.channels.SocketChannel
import scala.actors.Actor
import java.nio.ByteBuffer
import java.nio.charset.Charset
import java.nio.CharBuffer
import java.nio.charset.CharsetDecoder
import java.nio.charset.CoderResult
import src.com.ssb.bbs.AnsiScreen




class Client(socket:SocketChannel) extends Actor {

	private var currentUser:User = null;
	private var socketDead = false;
	
	private var telnetProtocol: TelnetProtocol = null;
	private var screen: AnsiScreen = null;
	
	def act() = {
		
		telnetProtocol = new TelnetProtocol(socket);
		screen = new AnsiScreen(telnetProtocol);
				
		
		//telnetProtocol.readTerminalType();
		telnetProtocol.echo(false);
		telnetProtocol.supressGoAhead(false);
		
		//telnetProtocol.update();
		
		telnetProtocol.readWindowSize((a:Int, b:Int) => println("%d %d".format(a, b)));
		
		telnetProtocol.put(ByteBuffer.wrap(Array(0x1b, '[', '2', 'J')));
		
		telnetProtocol.put(ByteBuffer.wrap(Array(0x1b, '[', '2', ';', '2', 'H')));
		
		screen.put("LOGIN:");
		val name = screen.getLine();
		println("NAME: '" + name + "'");
		screen.put("PASSWORD:");
		val pass = screen.getLine();

		var result = UserService !! Login(name, pass);		
		screen.put("PROCESSING LOGIN\n");
		var user = result().asInstanceOf[User];
		if(user != NoUser) {
			screen.put("Login successful: " + user.name + "!\n");
			currentUser = user;
		} else {
			screen.put("Login failed\n");
		}
		
		while(true) {
			val command = screen.getLine("-->");
			println(command);
		}
	}
}
