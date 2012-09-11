package com.ssb.bbs

import java.nio.ByteBuffer
import java.nio.channels.Channel
import java.nio.channels.SocketChannel
import java.nio.charset.Charset
import java.nio.CharBuffer
import java.lang.System
import java.io.PipedReader
import java.io.PipedWriter
import java.io.BufferedReader

object TelnetProtocol {
	val charset = Charset.forName("ISO-8859-1");
}

class TelnetProtocol(channel: SocketChannel) {
	
	val IAC:Byte = -1;
	val DONT:Byte = -2;
	val DO:Byte = -3;
	val WONT:Byte = -4;
	val WILL:Byte = -5;
	val SB:Byte = -6;
	val SE:Byte = -10;
	
	val NORMAL = 0;
	val FOUND_IAC = 1;
	val SUB_OPTION = 2;
	val FOUND_IAC_SUB = 3;
	
	var option:Int = 0;
	var what:Int = 0;
	
	private val buffer = ByteBuffer.allocateDirect(1024);
	private val tempBuffer = ByteBuffer.allocateDirect(1024);
	private val optionData = ByteBuffer.allocateDirect(128);
	//private val outBuffer = CharBuffer.allocate(1024);
	private var byteStack  = List[Byte]();
	private var state = NORMAL;
	
	private val decoder = TelnetProtocol.charset.newDecoder();
	private val encoder = TelnetProtocol.charset.newEncoder();
	
	private val pipeWriter = new PipedWriter();
	private val pipeReader = new PipedReader(pipeWriter);
	private val bufferedReader = new BufferedReader(pipeReader);
	
	
	def update() {
		
		val count = channel.read(buffer);
		println("COUNT: %d\n".format(count));
		buffer.flip();
		
		while(buffer.remaining() > 0) {
			val b = buffer.get();
			println("%02x\n".format(b));
			
			if(state == SUB_OPTION) {
				if(b == IAC)
					state = FOUND_IAC_SUB;
				else
					optionData.put(b);				
			}
			else
			if(state == FOUND_IAC_SUB) {
				if(b == SE)
					state = NORMAL;
				else {
					state = SUB_OPTION;
					optionData.put(IAC);
					optionData.put(b);
				}
			}
			else
			if(state < 0) {
				what = state;
				option = b;
				if(state == SB)
					state = SUB_OPTION;
				else
					state = NORMAL;
			}
			else
			if(state == FOUND_IAC) {
				if(b < 0 && b >= SE)
					state = b;
				else {
					state = NORMAL;
					tempBuffer.put(IAC);
					tempBuffer.put(b);
				}
			} 
			else		
			if(b == IAC) {
				state = FOUND_IAC;				
			}
			else
			if(state == NORMAL) {
				tempBuffer.put(b);
			}
		}
		buffer.clear();
		
		if(tempBuffer.position() > 0) {
			tempBuffer.flip();		
			val cb:CharBuffer = decoder.decode(tempBuffer);
			pipeWriter.write(cb.array());
			tempBuffer.clear();
		}
	}
		
	
	def readChar() : Char = {		
		update();
		return bufferedReader.read().asInstanceOf[Char];
	}
	
	def readLine(): String = {
		update();
		return bufferedReader.readLine();
	}
	
	def put(s:String) = {
		val output = encoder.encode(CharBuffer.wrap(s));
		channel.write(output);
	}

	def put(s:ByteBuffer) = {
		channel.write(s);
	}

}