I wanted to try to write a rat that would have basic functionality, which is executing any commands in cmd/powershell and swapping the file from outside. The built-in windows libraries were also used exclusively.
The control is very simple, every 20 seconds the bot checks for updates in the bot.
You can set a task for him via the /task command (argument).
For example /task tasklist, it will perform this task every 20 seconds until you set the task /task skip, there is also a separate command /task download_by_url link, after it is executed,
it will download any file to the folder "C:\\Users ". Since RAT is installed as a windows service,
it will be launched every time the PC starts. The installation is very simple, run builder.py , specify your userid in telegram and the bot token, after that it will compile two files under the names start.exe and main.exe.
To install the service on a PC, run start.exe on behalf of the administrator, you can delete files after that.
Make sure that you have installed g++ and pyinstaller, this is necessary to compile the service.
