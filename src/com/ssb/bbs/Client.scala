package com.ssb.bbs

import java.nio.channels.SocketChannel
import scala.actors.Actor
import java.nio.ByteBuffer
import java.nio.charset.Charset
import java.nio.CharBuffer
import java.nio.charset.CharsetDecoder
import java.nio.charset.CoderResult




class Client(socket:SocketChannel) extends Actor {

	private var currentUser:User = null;
	private var socketDead = false;
	
	private var telnetProtocol: TelnetProtocol = null;

	
	private def getLine(prompt:String = null) : String = {
		
		if(prompt != null)
			put(prompt);
		
		//telnetProtocol.echo = true;
		
		val sb = new StringBuilder();
		
		var end = false;
		while(!end) {
			var c = telnetProtocol.readChar();

			c match {
				case 10 => end = true;

				case 0x7f => if(sb.length > 0) {
					sb.deleteCharAt(sb.length-1); c = 8	
				} else c = 0;

				case _ => sb.append(c);
			}
			if(c > 0)
				telnetProtocol.putChar(c.asInstanceOf[Char]);
		}
		
		//telnetProtocol.echo = false;
		
		val line:String = sb.toString();
		/*if(line == null) {
			println("EOF!");
			if(currentUser != null)
				UserService ! Logout(currentUser);
			socketDead = true;
			exit();
		} */
		
		
		
		return line;
	}
	
	private def put(s:String) {
		telnetProtocol.put(s);
	}
	
	def act() = {
		
		telnetProtocol = new TelnetProtocol(socket);
				
		
		//telnetProtocol.readTerminalType();
		telnetProtocol.echo(false);
		telnetProtocol.supressGoAhead(false);
		
		//telnetProtocol.update();
		
		telnetProtocol.readWindowSize((a:Int, b:Int) => println("%d %d".format(a, b)));
		
		telnetProtocol.put(ByteBuffer.wrap(Array(0x1b, '[', '2', 'J')));
		
		telnetProtocol.put(ByteBuffer.wrap(Array(0x1b, '[', '2', ';', '2', 'H')));
		
		put("LOGIN:");
		val name = getLine();
		println("NAME: '" + name + "'");
		put("PASSWORD:");
		val pass = getLine();

		var result = UserService !! Login(name, pass);		
		put("PROCESSING LOGIN\n");
		var user = result().asInstanceOf[User];
		if(user != NoUser) {
			put("Login successful: " + user.name + "!\n");
			currentUser = user;
		} else {
			put("Login failed\n");
		}
		
		while(true) {
			val command = getLine("-->");
			println(command);
		}
	}
}
