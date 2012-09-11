package com.ssb.bbs

trait Protocol {

	def readByte():Byte;
	def putByte(b: Byte);

	def put(bb: ByteBuffer) {
		for(b <- bb.array()) putByte(b);
	}
}