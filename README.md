## CS425 MP3 - Distributed Failure Detector

  

Follow these instructions to run the Hybrid Distributed File System -

  

1. Run following commands on each of the node - 
git clone git@gitlab.engr.illinois.edu:abandal2/g73_mp2.git
and checkout to branch mp3-demo
2. Build the main.cpp program on all machines on your distributed system using
	make
3. Daemon executables would be build with g++ compiler
4. Run the introducer on one of the machine using pre configured port number using
    ./Daemon  $updPort  $tcpPort  introducerVMName  remote  introducer
5. Run the daemon on all other machines using following command
	./Daemon  $updPort  $tcpPort  introducerVMName  remote
6. To check the list of nodes in the HyDFS system enter list_mem_ids
7. Store any local files in ./files/remote/local directory
8. Run create localFileName hyDFSFileName to create hyDFSFileName in the HyDFS
9. Run get hyDFSFileName localFileName from any node to fetch hyDFSFileName to the local storage



Alternatively, to build daemon on all machines run bash init.sh. Make sure to follow instruction and change paths to secret tokens and passwords in the script. You can also configure port number for server in the file.