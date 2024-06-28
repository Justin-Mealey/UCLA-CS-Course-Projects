#!/bin/bash

correct=false

exampleinput=$((1 + $RANDOM % 100000)) #test randall with arbitrary number 1-100,000

exampleoutputlength=$(./randall $exampleinput | wc -c) 

#output length should be exampleinput number of bytes, so our two vars should be =
if [ $exampleinput = $exampleoutputlength ]
then
    echo "Succeeds basic test"
else
    echo "Fails basic test"
fi

e2=$(./randall $exampleinput -i rdrand | wc -c)
if [ $exampleinput = $e2 ]
then
    echo "Succeeds rdrand test"
else
    echo "Fails rdrand test"
fi

e3=$(./randall $exampleinput -i lrand48_r | wc -c)
if [ $exampleinput = $e3 ]
then
    echo "Succeeds lrand48_r test"
else
    echo "Fails lrand48_r test"
fi

e4=$(./randall $exampleinput -o 5 | wc -c)
if [ $exampleinput = $e4 ]
then
    echo "Succeeds N test"
else
    echo "Fails N test"
fi

e5=$(./randall $exampleinput -o stdio | wc -c)
if [ $exampleinput = $e5 ]
then
    echo "Succeeds stdio test"
else
    echo "Fails stdio test"
fi

e6=$(./randall $exampleinput -o stdio -i lrand48_r | wc -c)
if [ $exampleinput = $e6 ]
then
    echo "Succeeds combination test"
else
    echo "Fails combination test"
fi

e7=$(./randall $exampleinput -i /dev/random | wc -c)
if [ $exampleinput = $e4 ]
then
    echo "Succeeds file test"
else
    echo "Fails file test"
fi