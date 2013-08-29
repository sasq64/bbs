package com.ssb.bbs

import scala.actors.Actor

case class Msg;
case class Login(name:String, pass:String) extends Msg;
case class Logout(user:User) extends Msg;

case class User(name:String);

case object NoUser extends User(null);

object UserService extends Actor {
	
	private var loggedIn = List[User]();
	
	def act() {
		loop { react { /**/
			case Login(name, pass) => {
				println("Trying to log in:" + name);
				if(name == "root" && pass == "toor") {
					val user = new User("Jonas");
					loggedIn = user :: loggedIn;
					sender ! user; 
				} else {
					sender ! NoUser;
				}
			}
			case Logout(user) => {
				println("Logged out:" + user.name + "\n");
				loggedIn = loggedIn.filterNot { _.name == user.name };
			}
		}} /**/
	}
}
