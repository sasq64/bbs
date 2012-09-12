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

class TelnetProtocol(channel: SocketChannel) extends Protocol {
	
val IAC:Byte = -1; //ff
	val DONT:Byte = -2; //fe
	val DO:Byte = -3; //fd
	val WONT:Byte = -4; //fc
	val WILL:Byte = -5;	 //fb
	val SB:Byte = -6;
	val GA:Byte = -7;
	val EL:Byte = -8;
	val EC:Byte = -9;
	val AYT:Byte = -10;
	val AO:Byte = -11;
	val IP:Byte = -12;
	val BRK:Byte = -13;
	val DM:Byte = -14;
	val NOP:Byte = -15;
	val SE:Byte = -16;

	val LF:Byte = 10;
	val CR:Byte = 13;
	
	object Mode extends Enumeration {
		type Mode = Value;
		val NORMAL, FOUND_IAC, SUB_OPTION, FOUND_IAC_SUB, CR_READ, OPTION = Value;
	}
	import Mode._
	
	/* val NORMAL = 0;
	val FOUND_IAC = 1;
	val SUB_OPTION = 2;
	val FOUND_IAC_SUB = 3;
	val CR_READ = 4;
	val OPTION = 5; */
	
	val TRANSMIT_BINARY:Byte = 0;
	val ECHO:Byte = 1;
	val SUPRESS_GO_AHEAD:Byte = 3;
	val TERMINAL_TYPE:Byte = 24;
	val WINDOW_SIZE:Byte = 31;
	
	var option:Int = 0;
	var what:Int = 0;
	
	var width:Int = -1;
	var height:Int = -1;
	
	private var windowSizeCallback:Function2[Int, Int, Unit] = null;
	
	private val buffer = ByteBuffer.allocateDirect(1024);
	private val outBuffer = ByteBuffer.allocateDirect(1024);
	private val optionData = ByteBuffer.allocateDirect(128);
	private val ctempBuffer = CharBuffer.allocate(1024); 
	
	case class State(var mode:Mode, var byte:Byte = -1);
	
	private var state:State = new State(NORMAL, -1);
	
	var terminalType:String = null;
	
	def echo(on: Boolean) {
		val bb = ByteBuffer.wrap(Array(IAC, WILL, ECHO));
		put(bb);
	}
	
	def supressGoAhead(on: Boolean) {
		val bb = ByteBuffer.wrap(Array(IAC, WILL, SUPRESS_GO_AHEAD));
		put(bb);		
	}
	
	def readTerminalType() {
		val bb = ByteBuffer.wrap(Array(IAC, DO, TERMINAL_TYPE));
		put(bb);				
	}
	
	def readWindowSize(cb:Function2[Int, Int, Unit]) {
		val bb = ByteBuffer.wrap(Array(IAC, DO, WINDOW_SIZE));
		windowSizeCallback = cb;
		put(bb);				
	}

	private def setOption(w:Int, o:Int) = {
		what = w ; 
		option = o;
		println("OPTION %d/%d".format(w,o));
		
		if(what == WILL) {
			if(option == TERMINAL_TYPE) {
				val bb = ByteBuffer.wrap(Array(IAC, SB, TERMINAL_TYPE, 1, IAC, SE));
				put(bb);
			}
		}
	}
	
	private def handleOptionData() {
		 if(option == WINDOW_SIZE) {
			width = optionData.get(0) << 8 | (optionData.get(1)&0xff);
			height = optionData.get(2) << 8 | (optionData.get(3)&0xff);
			println("Window size %d x %d".format(width, height));
			if(windowSizeCallback != null)
				windowSizeCallback(width, height);
			
		} else
		if(option == TERMINAL_TYPE) {
			optionData.get();
			//terminalType = decoder.decode(optionData).toString();
		}
		optionData.clear();
	}
	
	private def update() {
		
		val count = channel.read(buffer);
		println("COUNT: %d\n".format(count));
		buffer.flip();
		
		while(buffer.remaining() > 0) {

			state.byte = buffer.get();
			
			//val b = buffer.get();
			println("%02x\n".format(state.byte));
			
			state.mode = state match {
				case State(SUB_OPTION, IAC) => FOUND_IAC_SUB
				case State(SUB_OPTION, b) => optionData.put(b); SUB_OPTION
				case State(FOUND_IAC_SUB, SE) => handleOptionData(); NORMAL
				case State(FOUND_IAC_SUB, b) => optionData.put(IAC); optionData.put(b); SUB_OPTION
				case State(OPTION, b) if(option == SB) => setOption(SB, b);  SUB_OPTION
				case State(OPTION, b) => setOption(option, b);  NORMAL
				case State(FOUND_IAC, b) if(b < 0 && b >= SE) => option = b; OPTION
				case State(FOUND_IAC, b) => outBuffer.put(IAC); outBuffer.put(b); NORMAL
				case State(NORMAL, IAC) => FOUND_IAC
				case State(CR_READ, 0) => outBuffer.put(LF) ; NORMAL
				case State(CR_READ, 10) => outBuffer.put(LF) ; NORMAL
				case State(CR_READ, b) => outBuffer.put(CR) ; NORMAL
				case State(NORMAL, CR) => CR_READ
				case State(NORMAL, b) => outBuffer.put(b); NORMAL
			}
			
			println("BYTE: %d => STATE: %s".format(state.byte, state.mode.toString()));
			
		}
		buffer.clear();
	}
	
	override def read(bb: ByteBuffer) {
		update();
		outBuffer.flip();
		bb.put(outBuffer);
		outBuffer.compact();
	}
		
	
	override def readByte() : Byte = {		
		update();
		outBuffer.flip();
		val b = outBuffer.get();
		outBuffer.compact();
		return b;
	}
	
	override def putByte(b: Byte) {
		if(b == LF) {
			val bb = ByteBuffer.wrap(Array(CR, LF));
			channel.write(bb);			
		} else {
			val bb = ByteBuffer.wrap(Array(b));
			channel.write(bb);
		}
	}

}