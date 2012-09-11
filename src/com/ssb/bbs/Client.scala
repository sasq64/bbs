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
		val line:String = telnetProtocol.readLine();
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
		
		val bb = ByteBuffer.wrap(Array(-1,-5,1,-1,-5,3));
		telnetProtocol.put(bb);		
		telnetProtocol.update();
		
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
			put("Login failed");
		}
		
		while(true) {
			val command = getLine("-->");
		}
	}
}
