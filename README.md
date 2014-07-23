remotesutdownPc
===============
Only fo windows system
Remote Shutdown the windows system when a Magic Packet received
tips:1. the program need to be runned with administrator privilege
after compiling  and geting the program. you should use the SC command to install this program as a service.
for example  SC create remoteshutdown binpath= "c:\remoteshutdown.exe" depend= Tcpip Start= auto

the Magic Packet is used to remote start the pc. And when the system is already running the Magic Packet received will be ignored. Here the Magic Packet is used to remote shutdown the system, when a Magic packet is received.

this web site provide the service to send Magic Packet:http://www.depicus.com/wake-on-lan/woli.aspx
there are also a lot of app in the apple store for iphone, which can used to send a Magic packet
