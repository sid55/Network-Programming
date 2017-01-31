#!/usr/bin/python3

import os
import pexpect
from operator import eq
from signal import SIGKILL
import sys

port = "1234"
ip = "127.0.0.1"
server_name = os.path.abspath(os.path.join(os.getcwd(),
                                           "../bin/server"))
client_name = os.path.abspath(os.path.join(os.getcwd(),
                                           "../bin/client"))

testdir = "ex1testdir3571234612123421"
prompt = "> "
commands = [
    ("ls", ""),
    ("touch a", ""),
    ("ls", "a"),
    ("echo b", "b"),
    ("echo c > b", ""),
    ("ls", "a", "b"),
    ("sdkjf", ""),
    ("rm a", ""),
    ("ls", "b"),
    ("cat b", "c"),
    ("rm b", ""),
    ]

def fail(reason):
    print("FAILED: " + reason)
    import pdb; pdb.set_trace()
    exit()

def test(function, message, *args, **kwargs):
    try:
        r = function(*args, **kwargs)
        return r
    except Exception as e:
        fail("{}: {}".format(message, e))

def test_command(program, command, out):
    try:
        program.sendline(command)
        program.expect(prompt)
    except pexpect.EOF:
        fail("Client terminated unexpectedly after command: " + command)
    except pexpect.TIMEOUT:
        fail("Client did not respond to command: " + command)
    except:
        fail("Command '{}' caused error in client".format(command))
    lines = [l.strip() for l in program.before.splitlines()]
    l = [l.decode(sys.stdout.encoding) for l in lines]
    error_message = "Output does not correspond to expected for command {}.\nExpected:\n{}\n\nFound:\n{}\n".format(
        command, '\n'.join(out), '\n'.join(l))
    test(eq, error_message, out, lines)

try:
    os.makedirs(testdir)
except OSError:
    if not os.path.isdir(testdir):
        raise
dest = os.path.join(os.getcwd(), testdir)

# Test normal functionality
server = test(pexpect.spawn, "Server did not start properly",
              server_name, [port], cwd=dest)
client = test(pexpect.spawn, "Client did not start properly",
              client_name, [ip, port])

test(client.expect,  "Client did not start properly", prompt)

for t in commands:
    com = t[0]
    out = t[1:]
    test_command(client, com, out)
    if not server.isalive():
        fail("Server crashed after command '{}'".format(com))
    
test(client.sendline, "Error while exiting client", "exit")
if not server.isalive():
    fail("Server crashed after client closed")

# Test case that client crashes
client = test(pexpect.spawn, "Client did not start properly",
              client_name, [ip, port], echo=False)

breakpoint = int(len(commands) // 2)
for t in commands[:breakpoint]:
    test_command(client, com, out)
    if not server.isalive():
        fail("Server crashed after command '{}'".format(com))
    
client.kill(SIGKILL)

# Test connection after a client crashed
client = test(pexpect.spawn, "Client did not start properly",
              client_name, [ip, port], echo=False)

for t in commands[breakpoint:]:
    com = t[0]
    out = t[1:]
    test_command(client, com, out)
    if not server.isalive():
        fail("Server crashed after command '{}'".format(com))
test(client.sendline, "Error while exiting client", "exit")

print("All tests ran successfully.")
