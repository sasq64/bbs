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

object TelnetProtocol extends Protocol {
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
	private val outBuffer = ByteBuffer.allocateDirect(1024);
	private val optionData = ByteBuffer.allocateDirect(128);
	
	case class State(var state:Int, var byte:Byte = -1);
	
	private var state:State = new State(NORMAL, -1);
	
	private def setOption(w:Int, o:Int) = {
		what = w ; 
		option = o;
		println("OPTION %d/%d".format(w,o));
	}
	
	private def update() {
		
		val count = channel.read(buffer);
		println("COUNT: %d\n".format(count));
		buffer.flip();
		
		while(buffer.remaining() > 0) {

			state.byte = buffer.get();
			
			//val b = buffer.get();
			println("%02x\n".format(state.byte));
			
			state.state = state match {
				case State(SUB_OPTION, IAC) => FOUND_IAC_SUB
				case State(SUB_OPTION, b) => optionData.put(b); SUB_OPTION
				case State(FOUND_IAC_SUB, SE) => NORMAL
				case State(FOUND_IAC_SUB, b) => optionData.put(IAC); optionData.put(b); SUB_OPTION
				case State(s, b) if s < 0 => setOption(s, b);  NORMAL
				case State(FOUND_IAC, b) if(b < 0 && b >= SE) => b
				case State(FOUND_IAC, b) => outBuffer.put(IAC); outBuffer.put(b); NORMAL
				case State(NORMAL, IAC) => FOUND_IAC
				case State(NORMAL, b) => outBuffer.put(b); NORMAL
			}
			
			println("BYTE: %d => STATE: %d".format(state.byte,state.state));
			
		}
		buffer.clear();
	}
		
	
	def readByte() : Byte = {		
		update();
		return outBuffer.get();
	}

	def put(s:ByteBuffer) = {
		channel.write(s);
		update();
	}

}