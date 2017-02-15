#!/bin/bash

# execute from the root of your project containing src, bin etc subdirectories
#
# runs servers in ./test directory
# runs client in ./src directory

# This test is provided as is. It manipulates files while testing the
# programs, so make sure to use correctly to avoid accidental file loss.
# To use this test script, create a test directory at the same level as the
# src and bin directories, copy this script to that folder and run it with bash

# pick a filename in bin directory
FILENAME=ddfile1m
TESTDIR=test

NUM_CONN=3

# set -e

# should a src directory under here
# used as relative directory for paths
if pushd ./src
then
    if [ ! -d ../bin ]; then
        echo ERROR: No bin directory found
        exit 1
    fi

    if [ ! -d ../$TESTDIR ]; then
        mkdir ../$TESTDIR
    fi

    pushd ../$TESTDIR
    ../bin/myserver 1234 &
    ../bin/myserver 1235 &
    ../bin/myserver 1236 &

    dd if=/dev/zero of=0$FILENAME bs=1024 count=1024
    cat 0$FILENAME | tr '\0' '\177' > $FILENAME
    rm 0$FILENAME
    popd
else
    echo ERROR: No src directory found
    exit 1
fi


# set +e

# check for empty server-info file
cat << EOF > ./server-info.text
127.0.0.1 1234
127.0.0.1 1235
EOF

counter=0

echo Retrieving file $FILENAME
ls -l ../$TESTDIR/$FILENAME

#
../bin/myclient ./server-info.text $NUM_CONN $FILENAME || exit 1
out=$(diff $FILENAME ../$TESTDIR/$FILENAME)
rc=$?
if [[ $rc != 0 ]]
then
    echo "Test failed with two servers and three connections. (rc $rc)"
    counter=$((counter + 1))
else
    echo "Test success! (rc $rc)"
    rm $FILENAME
fi

# add a new server
echo "127.0.0.1 1236" >> ./server-info.text

../bin/myclient ./server-info.text $NUM_CONN $FILENAME
out=$(diff $FILENAME ../$TESTDIR/$FILENAME)
rc=$?
if [[ $rc != 0 ]]
then
    echo "Test failed with three servers and three connections. (rc $rc)"
    counter=$((counter + 1))
else
    echo "Test success! (rc $rc)"
    rm $FILENAME
fi

# add another server
echo "127.0.0.1 1237" >> ./server-info.text

../bin/myclient ./server-info.text $NUM_CONN $FILENAME
out=$(diff $FILENAME ../$TESTDIR/$FILENAME)
rc=$?
if [[ $rc != 0 ]]
then
    echo "Test failed with four servers and three connections. (rc $rc)"
    counter=$((counter + 1))
else
    echo "Test success! (rc $rc)"
    rm $FILENAME
fi

echo "Test finished, $counter error(s) found."

echo "Killing all servers"
killall myserver

pushd ../$TESTDIR
    echo "Removing test data file $filename"
    rm $FILENAME
popd
