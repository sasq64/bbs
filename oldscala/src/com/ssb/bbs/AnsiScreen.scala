package src.com.ssb.bbs

import java.nio.charset.Charset
import java.nio.CharBuffer
import com.ssb.bbs.Protocol
import com.ssb.bbs.Terminal
import java.nio.ByteBuffer

object AnsiScreen extends {
	val charset = Charset.forName("ISO-8859-1");
}

class AnsiScreen(protocol: Protocol) {

	private val decoder = AnsiScreen.charset.newDecoder();
	private val encoder = AnsiScreen.charset.newEncoder();
	
	private val charBuffer = CharBuffer.allocate(1024);
	private val outBuffer = ByteBuffer.allocate(1024);
	private val tempBuffer = ByteBuffer.allocate(1024);
		
	def putChar(c: Char) {
		val bb = AnsiScreen.charset.encode(c.toString);
		protocol.put(bb);
	}
	
	def update() {
		protocol.read(tempBuffer);
		println(charBuffer.position, charBuffer.limit);
		if(tempBuffer.position() > 0) {
			tempBuffer.flip();			
			decoder.decode(tempBuffer, charBuffer, false);
			tempBuffer.clear();
		}
		
	}
	
	def getChar(): Char = {
		while(charBuffer.position() == 0)
			update();
		charBuffer.flip();
		val c = charBuffer.get();
		charBuffer.compact();
		return c;
	}
	
	def put(s: String) {
		
		val bb = AnsiScreen.charset.encode(s);
		protocol.put(bb);
	}
	
	def getLine(prompt:String = null) : String = {
		
		if(prompt != null)
			put(prompt);
		
		//telnetProtocol.echo = true;
		
		val sb = new StringBuilder();
		
		var end = false;
		while(!end) {
			var c = getChar();

			c match {
				case 10 => end = true;

				case 0x7f => if(sb.length > 0) {
					sb.deleteCharAt(sb.length-1); c = 8	
				} else c = 0;

				case _ => sb.append(c);
			}
			if(c > 0)
				putChar(c.asInstanceOf[Char]);
		}

		val line:String = sb.toString();
		return line;
	}

	
}