#!/usr/bin/python
import pexpect
import os
import threading
os.environ["TERM"] = 'xterm'


def tryChat(name, line) :
	child = pexpect.spawn ('telnet localhost 12345')
	child.expect ('NAME:')
	child.sendline (name)
	child.expect ('PASSWORD:')
	child.sendline ('password')
	child.expect('chat')
	child.send('c')
	child.expect ('joined')
	child.sendline (line)
	child.expect (line)
	child.sendline ("MORE" + line)
	child.expect ("MORE" + line)

def tryShell(name, line) :
	child = pexpect.spawn ('telnet localhost 12345')
	child.expect ('NAME:')
	child.sendline (name)
	child.expect ('PASSWORD:')
	child.sendline ('password')
	child.expect('shell')
	child.send('s')
	child.expect ('exit')
	child.sendline ('ed bbstest.cpp')
	child.sendline (line)

for x in range(10) :
	t = threading.Thread(target=tryChat, args = ('joe' + str(x), 'hellopeople' + str(x)))
	t.start()

	t = threading.Thread(target=tryShell, args = ('moe' + str(x), 'fadshkjfdsah kjdsafdsa\nfsadfsa\nfasfdsafa\nfasfsadf\nfasfdsa\ndsadsa' + str(x)))
	t.start()
