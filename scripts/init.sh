#!/bin/bash


HOSTS=(
"fa24-cs425-7301.cs.illinois.edu"
"fa24-cs425-7302.cs.illinois.edu"
"fa24-cs425-7303.cs.illinois.edu"
"fa24-cs425-7304.cs.illinois.edu"
"fa24-cs425-7305.cs.illinois.edu"
"fa24-cs425-7306.cs.illinois.edu"
"fa24-cs425-7307.cs.illinois.edu"
"fa24-cs425-7308.cs.illinois.edu"
"fa24-cs425-7309.cs.illinois.edu"
"fa24-cs425-7310.cs.illinois.edu"
)

USERNAME="rahuls14"
CURR_BRANCH=$(git branch --show-current 2>/dev/null)
echo $CURR_BRANCH
REPO_URL="https://gitlab.engr.illinois.edu/abandal2/g73_mp2.git"

REPO_DIR="g73_mp2"
PASSWORD_FILE="/Users/rahul/password.txt"
ACCESS_TOKEN_FILE="/Users/rahul/gitlabAccessToken.txt"
GIT_ACCESS_TOKEN=$(cat "$ACCESS_TOKEN_FILE")
REPO_URL="https://cs425:${GIT_ACCESS_TOKEN}@gitlab.engr.illinois.edu/abandal2/g73_mp2.git"
REMOTE_COMMANDS="cd ~/g73_mp2"
LOGS_REPO="logs"
TEST_LOGS_REPO="test_logs"
PORT=4560
TCPPORT=7890
echo $REPO_URL


if [[ -f "$PASSWORD_FILE" ]]; then
    PASSWORD=$(<"$PASSWORD_FILE")
else
    echo "Password file not found!"
    exit 1
fi

i=1

for HOST in "${HOSTS[@]}"
do
    echo "Connecting to $HOST"

    sshpass -p $PASSWORD ssh -o StrictHostKeyChecking=no "$USERNAME@$HOST" bash -c "'


        ## pull git repo
        rm -r $REPO_DIR
        if [ ! -d \"$REPO_DIR\" ]; then
            echo \"Cloning repository on $HOST...\"
            git clone $REPO_URL $REPO_DIR
        fi

        cd $REPO_DIR
        echo \"pulling $HOST...\"
        git checkout $CURR_BRANCH
        # git pull origin main
        cd ~

        # build and run server

        cd $REPO_DIR 
        make
        PID=\$(lsof -t -i:$PORT)
        if [ -z "\$PID" ]; then
            echo "No process found running on port $PORT"
        else
            # Kill the process
            kill -9 \$PID
            echo "Process running on port has been killed"
        fi

        PID=\$(lsof -t -i:$TCPPORT)
        if [ -z "\$PID" ]; then
            echo "No process found running on port $TCPPORT"
        else
            # Kill the process
            kill -9 \$PID
            echo "Process running on port has been killed"
        fi

        exit
    '"

    # if [ $? -eq 0 ]; then
    #     echo "Commands executed successfully on $HOST."
    # else
    #     echo "Failed to execute commands on $HOST."
    # fi

    ((i++))
    echo "---------------------------------"
done
