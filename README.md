# sourcemod-async

Fast HTTP/S client for Sourcemod.

https://forums.alliedmods.net/showthread.php?t=237812

Uses 1 seperate thread to handle all requests with an event loop which should be safer and more reliable than using 1 thread per request.