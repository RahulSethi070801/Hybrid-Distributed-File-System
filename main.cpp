#include "Daemon.cpp"
#include <bits/stdc++.h>
using namespace std;


int main (int argc, char* argv[]) {
    // parse command line arguments and pass to Daemon - ./daemon <port> <introducer port>, introducer port is optional
    // ./Daemon 4560 9870 localhost local/remote introducer
    int introducerNumArgs = 6;
    if(argc>=5){
        bool isIntroducer = argc == introducerNumArgs ? true : false;
        string machineType = isIntroducer ? "introducer":"node";
        string env = argv[4];
        char *daemonPort = argv[1];
        char *hydfsPort = argv[2];
        char *introducerName = argv[3];
        cout<<"Starting "<<machineType<<" on "<<daemonPort<<" with introducer on "<<introducerName<<" with "<<env<<" environment\n";

        HyDFS dfs(hydfsPort);
        dfs.start();
        Daemon d(daemonPort, hydfsPort);
        d.addListener(&dfs);
        d.start(isIntroducer, introducerName);

        if(string(argv[4]) == "remote") {
            dfs.setLocalFilesPath("./files/remote");
        } else {
            if(isIntroducer){
                dfs.setLocalFilesPath("./files/introducer");
            } else {
                dfs.setLocalFilesPath("");
            }
        }
        
        while(1){
            string command;
            getline(cin, command);
            if(command == "leave"){
                dfs.stopThreads();
                d.stopThreads();
                break;
            } else {
                d.runCommand(command);
            }
        }
    } else {
        cout<<"Invalid arguments\n";
    }
    return 0;
}