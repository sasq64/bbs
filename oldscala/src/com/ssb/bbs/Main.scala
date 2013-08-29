package com.ssb.bbs

import java.net.InetSocketAddress
import java.nio.channels.ServerSocketChannel
import java.nio.channels.SocketChannel

object Main {

	def main(args: Array[String]) {

		val serverSocket = ServerSocketChannel.open();
		serverSocket.socket().bind(new InetSocketAddress(12345));

		//var clients = List[Client]();

		UserService.start();

		while (true) {
			val socket: SocketChannel = serverSocket.accept();
			val client: Client = new Client(socket);
			//clients = client :: clients;
			client.start();
		}
	}
}
