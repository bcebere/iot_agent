IoT agent
=================================
A small agent for IoTs.

Mirai botnet:
- It infects hosts with weak telnet passwords. It uploads itself via wget, tftpt or a small utility wget-alike(that is small enough to be pushed over telnet).
- On start, it scans the device and:
 * it kills communication ports(22, 23,80).
 * it hooks /dev/watchdog in order to prevent soft reboots.
 * it scans the memory for patterns from other botnets and kills their pids.

In case of Mirai(or other forks) infection, it is almost impossible to detect/block an ongoing attack from another network device(based on the above description).

One idea is to push an agent to an IoT the same way Mirai would infect it.

Advantages:
 - We could gather information about the internal IoT data.
 - We could actively prevent botnets from altering devices.
 - If the deployment is impossible(strong credentials/no opened ports), the device should be safe from an infection too.

Current implementation:
* the uploader binary tests a telnet connection to a device, detects its architecture, searches for a writable folder, and tries to start the bdiot daemon.
* the bdiot daemon is a small static binary, with no dynamic dependencies, that is compiled for multiple platforms. On start, it starts to send a heartbeat request every 30 seconds to an webserver(the main 'brain').
* via heartbeat response, the server can push to the agent different commands/ask for forensics. As for now, the agent supports scanning the up&running processes for a pattern, killing a process by pid or by the ports it uses.


Structure
---------

	/
	|- daemon - the agent and a webserver that can push commands via HTTP
	|- uploader - the tool that deploys the agent via telnet
	|- download - an experimental tool for platforms that don't have wget/tftp support
	|- README.md
