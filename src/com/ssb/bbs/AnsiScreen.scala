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
	
	private val ctempBuffer = CharBuffer.allocate(1024);
	private val outBuffer = ByteBuffer.allocate(1024);
	private val inBuffer = ByteBuffer.allocate(1024);
	
	def putChar(c: Char) {
		ctempBuffer.clear();
		ctempBuffer.put(c);
		encoder.encode(ctempBuffer, outBuffer, false);
		protocol.put(outBuffer);
	}
	
	def getChar(): Char = {
		ctempBuffer.clear();
		while(ctempBuffer.position() == 0) {
			protocol.read(inBuffer);
			decoder.decode(inBuffer, ctempBuffer, false);
		}
		return ctempBuffer.get();
	}
	
	def put(s: String) {
		ctempBuffer.clear();
		ctempBuffer.put(s);
		encoder.encode(ctempBuffer, outBuffer, false);
		protocol.put(outBuffer);		
	}
	
	def getLine(prompt:String = null) : String = {
		
		if(prompt != null)
			put(prompt);
		
		//telnetProtocol.echo = true;
		
		val sb = new StringBuilder();
		
		var end = false;
		while(!end) {
			var c = readChar();

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