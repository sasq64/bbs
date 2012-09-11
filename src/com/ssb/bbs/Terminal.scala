package com.ssb.bbs

import java.nio.charset.Charset
import java.nio.ByteBuffer
import java.nio.CharBuffer

trait Terminal {
	
	val EOL = 10;
	var protocol:Protocol;
	
	case class Color(r: Int, g: Int, b: Int);
	
	val ansiColors = List(
		Color(0,0,0),
		Color(128,0,0),
		Color(0,128,0),
		Color(128,128,0),
		Color(0,0,128),
		Color(128,0,128),
		Color(0,128,128),
		Color(192,192,192),
		Color(128,128,128),
		Color(255,0,0),
		Color(0,255,0),
		Color(255,255,0),
		Color(0,0,255),
		Color(255,0,255),
		Color(0,255,255),
		Color(255,255,255)
	)
	
	def gotoxy(x:Int, y:Int);
	def setColor(col: Color);
	def setBGColor(col: Color);

	private val charset = Charset.defaultCharset();
	
	private val decoder = charset.newDecoder();
	private val encoder = charset.newEncoder();

	private val bb = ByteBuffer.allocate(16);
	private val cb = CharBuffer.allocate(16);

	def getChar():Char = {
		
		cb.compact();
		while(cb.position() == 0) {
			bb.put(protocol.readByte);				
			val cr = decoder.decode(bb, cb, false);
		}
		
		cb.flip();
		return cb.get();

	}

	def putChar(c: Char);

	
	def setColor(index: Int):Unit = setColor(ansiColors(index))
	def setBGColor(index: Int):Unit = setBGColor(ansiColors(index))
	def putString(s: String) = s.foreach(putChar)
	
	def readLine(): String = {
		val sb = new StringBuilder();
		while(true) {
			val c = getChar();
			if(c != EOL)
				sb.append(c);
			else
				return sb.toString;
		}
		return null;
	}
	

}