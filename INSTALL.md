This is a brief installation guide. A C compiler is generally required for the installation. In these instructions, we use the “clang” compiler, which is available on Linux, macOS and Windows.

Under Linux and macOS, compile the file in the terminal with the following command:
clang -m64 localtalk.c -o localtalk

After successful compilation, you can open the file with:
./localtalk
execute the file. 
The program requires 1 argument when running. Use:
./localtalk -s
to send data from your computer to another computer.
Use the following command:
./localtalk -l
to switch your computer to “listening mode”. This means that after successfully executing the command, you can receive data from another computer on which you executed the first command.

Finally, it is important to emphasize that the two computers must be on the same wifi.

NOTE:
To enable peer-to-peer connections in your local network, you may need to adjust your firewall settings. This project uses a custom TCP port (default: 8080) to communicate directly between devices.

Most firewalls block incoming connections by default, which can cause connection timeouts if you try to connect to a device running this app, especially on Linux and macOS systems using tools like ufw or firewalld.

Why this matters:
Your device must accept incoming TCP connections on the port used by this app (default: 8080). Otherwise, connection attempts from other devices on your LAN will fail, even if everything else is configured correctly.

What to do:
If you are using a firewall, allow incoming TCP traffic on port 8080, but only from your local network for safety.

Example (Linux with UFW):

sudo ufw allow from 192.168.0.0/16 to any port 8080 proto tcp
This allows incoming connections to port 8080 only from devices in the local network (LAN).

Other firewalls:

For other firewall systems (like Windows Firewall or firewalld), search for how to open a specific TCP port for local connections only.
