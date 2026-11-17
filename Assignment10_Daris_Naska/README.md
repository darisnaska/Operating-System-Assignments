gwgd - guess the word game daemon

gwgd is a small TCP server for a word guessing game. Each client is sent a short text with one word replaced by underscores and has to guess the missing word. Texts come from fortune -s; if fortune is not installed, a few built-in phrases are used instead.

gwg is the same game played on stdin/stdout, without networking.

Build
make

This produces gwgd and gwg. make clean removes them.

Run
gwgd [-t] [-f] [-p port]
Option	Meaning
(none)	Serve one client at a time; others wait in the queue
-f	Fork a child process for every client
-t	Start a detached thread for every client
-p port	Port to listen on (default 1234)

-f and -t cannot be combined. The server listens on all IPv4 and IPv6 addresses.

Play
$ nc localhost 1234
M: Guess the missing ____!
M: Send your guess in the form 'R: word\r\n'.
C: You are standing on my ____.
R: toes
O: Congratulation - challenge passed!
C: There is a fly __ your nose.
R: in
F: Wrong guess 'in' - expected 'on'
C: Is ____ really happening?
Q:
M: You mastered 1/2 challenges. Good bye!
Protocol

Lines end in \r\n. The first letter says what kind of message it is.

Server to client:

Prefix	Meaning
M:	Informational message
C:	Challenge: text with a hidden word
O:	Your guess was correct
F:	Your guess was wrong (shows the answer)

Client to server:

Message	Meaning
R: word	Your guess (case-insensitive)
Q:	Quit; the server reports your score

Anything else gets an M: hint and the same challenge is repeated. If the client disconnects, it counts as Q:.

Source layout
File	Purpose
chlng.c, chlng.h	Fetch text, pick and hide a word
player.c, player.h	Game logic and protocol messages (transport-free)
tcp.c, tcp.h	IPv4/IPv6 listening sockets, write-all helper
gwgd.c	Server: option parsing, select loop, the three modes
gwg.c	Standalone stdin/stdout game

The game logic is shared by every mode; only serve() in gwgd.c differs between blocking, fork and thread operation. In fork mode a SIGCHLD handler reaps children, so no zombies are left behind.
