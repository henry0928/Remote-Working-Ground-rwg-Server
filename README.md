# 2022-Fall-NYCU-Network-Programming-project2

In this project, you are asked to design 3 kinds of servers:
1. Design a Concurrent connection-oriented server. This server allows one client connect to it.
2. Design a server of the chat-like systems, called remote working systems (rwg). In this system, users can communicate with
other users. You need to use the single-process concurrent paradigm to design this server.
3. Design the rwg server using the concurrent connection-oriented paradigm with shared memory and FIFO.
These three servers must support all functions in project 1.

see more detail in NP_Project2_Spec_v2

這隻project份量有點大 時間不太夠的情況下檔案名稱沒有命名的很好
server1 -> np_simple.cpp npshell_forserver.cpp npshell_forserver.h
server2 -> np_single_proc.cpp npshell_singleprocess.cpp npshell_singleproccess.h
server3 -> np_multi_proc.cpp npshellforserver3.cpp npshellforserver3.h
