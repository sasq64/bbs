package src.com.ssb.bbs

import java.nio.charset.Charset
import java.nio.CharBuffer
import com.ssb.bbs.Protocol

object AnsiScreen {
	val charset = Charset.forName("ISO-8859-1");
}

class AnsiScreen(protocol: Protocol) {

	private val decoder = AnsiScreen.charset.newDecoder();
	private val encoder = AnsiScreen.charset.newEncoder();
	
	private val ctempBuffer = CharBuffer.allocate(1024);

	
}