#!/bin/bash
cd c:
cd cygwin/
cd AMOS/
./bootstrap > /cygdrive/c/cygwin/winXPFirst.log
if [ $? -ne 0 ]
then
cp /cygdrive/c/cygwin/winXPFirst.log /cygdrive/c/cygwin/winXPFirst_Failed.log
echo "FAILED: ./bootstrap" >> /cygdrive/c/cygwin/winXPFirst_Failed.log
/usr/bin/expect <<EOD
spawn scp /cygdrive/c/cygwin/winXPFirst_Failed.log ssh@sauron.cs.umd.edu:VMlogs
expect "ssh@sauron.cs.umd.edu's password:"
send "123\r"
expect eof
EOD
shutdown /s
fi

./configure --prefix=/usr/local/AMOS >> /cygdrive/c/cygwin/winXPFirst.log
if [ $? -ne 0 ]
then
cp /cygdrive/c/cygwin/winXPFirst.log /cygdrive/c/cygwin/winXPFirst_Failed.log
echo "FAILED: ./configure" >> /cygdrive/c/cygwin/winXPFirst_Failed.log
/usr/bin/expect <<EOD
spawn scp /cygdrive/c/cygwin/winXPFirst_Failed.log ssh@sauron.cs.umd.edu:VMlogs
expect "ssh@sauron.cs.umd.edu's password:"
send "123\r"
expect eof
EOD
shutdown /s
fi

make >> /cygdrive/c/cygwin/winXPFirst.log 
if [ $? -ne 0 ]
then
cp /cygdrive/c/cygwin/winXPFirst.log /cygdrive/c/cygwin/winXPFirst_Failed.log
echo "FAILED: make" >> /cygdrive/c/cygwin/winXPFirst_Failed.log
/usr/bin/expect <<EOD
spawn scp /cygdrive/c/cygwin/winXPFirst_Failed.log ssh@sauron.cs.umd.edu:VMlogs
expect "ssh@sauron.cs.umd.edu's password:"
send "123\r"
expect eof
EOD
shutdown /s
fi

make install >> /cygdrive/c/cygwin/winXPFirst.log
if [ $? -ne 0 ]
then
cp /cygdrive/c/cygwin/winXPFirst.log /cygdrive/c/cygwin/winXPFirst_Failed.log
echo "FAILED: make install" >> /cygdrive/c/cygwin/winXPFirst_Failed.log
/usr/bin/expect <<EOD
spawn scp /cygdrive/c/cygwin/winXPFirst_Failed.log ssh@sauron.cs.umd.edu:VMlogs
expect "ssh@sauron.cs.umd.edu's password:"
send "123\r"
expect eof
EOD
shutdown /s
fi
now=$(date +"%y%m%d")
echo "SUCCESS: complete log stored on http://sauron.cs.umd.edu/$now" >> /cygdrive/c/cygwin/winXPFirst.log
/usr/bin/expect <<EOD
spawn scp /cygdrive/c/cygwin/winXPFirst.log ssh@sauron.cs.umd.edu:VMlogs
expect "ssh@sauron.cs.umd.edu's password:"
send "123\r"
expect eof
EOD
rm /cygdrive/c/cygwin/winXPFirst.log
shutdown /s

