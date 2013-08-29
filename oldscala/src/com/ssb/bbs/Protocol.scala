package com.ssb.bbs

import java.nio.ByteBuffer

trait Protocol {

	def readByte():Byte;
	def putByte(b: Byte);

	def put(bb: ByteBuffer) {
		for(b <- bb.array()) putByte(b);
	}
	
	def read(bb: ByteBuffer);
	

}