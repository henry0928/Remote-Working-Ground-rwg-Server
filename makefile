all:np_simple.cpp
	g++ -g -o np_simple np_simple.cpp npshell_forserver.cpp
	g++ -g -o np_single_proc np_single_proc.cpp npshell_singleprocess.cpp
	g++ -g -o np_multi_proc np_multi_proc.cpp npshellforserver3.cpp -lrt -pthread 
