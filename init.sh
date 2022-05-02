#!/bin/bash
if [[ $# != 1 ]]; then
echo "Usage: $0 <file with list>";
exit 1;
fi;

f=$1;

groupadd officers;
groupadd clients;

while read -ra line;
do
    let i=0;
    username=" ";
    for word in "${line[@]}";
    do
        let i=i+1;
        if [ $i = 1 ]; then
            username=$word;
        elif [ $i = 2 ]; then
            echo $username;
            echo ${word}s;

            useradd $username;
            echo -e "$username\n$username" | passwd $username
            usermod -aG ${word}s $username;
        fi;
    done;
done < "$f";

rm -r officerDir/deposits
rm -r officerDir/credits
mkdir officerDir/deposits
mkdir officerDir/credits


setfacl -m group:clients:rwx officerDir/deposits;
setfacl -m group:clients:rwx officerDir/credits;

setfacl -d -m group:clients:--- officerDir/deposits;
setfacl -d -m group:clients:--- officerDir/credits;

setfacl -m group:officers:rwx officerDir/deposits;
setfacl -m group:officers:rwx officerDir/credits;

setfacl -d -m group:officers:rwx officerDir/deposits;
setfacl -d -m group:officers:rwx officerDir/credits;
