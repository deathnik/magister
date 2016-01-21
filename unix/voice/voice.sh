#!/usr/bin/env bash

if [ -z "$1" ] || [ "$1" == "-h" ]; then
    echo "Usage: $0 <path_to_file_with_data>" 1>&2
    exit 1
fi

filename=$1
tmp_name=$(mktemp)

# get data if file missing
if [ ! -f "$filename" ]
then
    filename=$tmp_name
    wget https://raw.githubusercontent.com/xbmc/xbmc/master/LICENSE.GPL -q -O "$filename"
fi

log="errors.log"
echo "" > $log

ok=0
neok=0

say_something(){
	festival -b "(begin  (SayText \"$1\" nil))" ;
}

clean_string(){
	a=$(echo "$1" | tr '[:punct:]' ' ' | tr -s ' ')
	echo "${a,,}"
}

on_ok(){
	say_something "Well done!"
	ok=$((ok+1))
}

on_error(){
	expected=$1
	typed=$2
	say_something "Expected: $expected"
	echo "Expected: $expected" >> $log
	say_something "Typed: $typed"
	echo "Typed: $typed" >> $log
	neok=$((neok+1))
}


welcome="Type the phrases you hear. Type a single question mark character to repeat, empty line to finish"
say_something "$welcome"

while true; do
	# get random line
	line=$(shuf -n 1 "$filename")
	while true ; do
		say_something "$line" ;
		#do not show typed text
       	read -s -r answer </dev/tty
       	if [ "$answer" != "?" ];
       	then
       		break;
       	fi
    done
    if [ "$answer" == "" ];
    then
    	break;
    fi
    #process answer
    clean_answer=$(clean_string "$answer") ;
    clean_line=$(clean_string "$line") ;
    if [ "$clean_answer" == "$clean_line" ];
    then
    	on_ok
    else
    	on_error "$clean_line" "$clean_answer"
    fi
done

#print error stat
sum=$((ok + neok))
if [ $sum -gt 0 ]
then
  errors_rate=$(bc <<< "scale=2; 100*$neok/($ok+$neok)")
  printf "\nErrors rate:$errors_rate  %%\n"
else
  printf "\nNo input done\n"
fi

# remove downloaded data
if [ "$filename" == "$tmp_name" ]
then
    rm "$tmp_name"
fi