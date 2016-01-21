#!/bin/bash
filename=$1

log="errors.log"
echo "" > $log

say_something(){
	festival -b "(begin  (SayText \"$1\" nil))" ;
}

clean_string(){
	a=$(echo "$1" | tr -d '[:punct:]' | tr -s ' ')
	echo "${a,,}"
}

on_ok(){
	say_something "Nice done!"

}

on_error(){
	expected=$1
	typed=$2
	say_something "Expected: $expected"
	echo "Expected: $expected" >> $log
	say_something "Typed: $typed"
	echo "Typed: $typed" >> $log
}


welcome="Type the phrases you hear. Type a single question mark character to repeat, empty line to finish"
#say_something "$welcome"

random_line=$(shuf -n 1 $filename)



while IFS='' read -r line || [[ -n "$line" ]]; do
	while true ; do
		say_something "$line" ;
       	read -r answer </dev/tty
       	if [ " $answer" != " ?" ];
       	then
       		break;
       	fi
    done
    if [ " $answer" == " " ];
    then
    	break;
    fi

    clean_answer=$(clean_string "$answer") ;
    clean_line=$(clean_string "$line") ;
    if [ " $clean_answer" == " $clean_line" ];
    then
    	on_ok
    else
    	on_error "$clean_line" "$clean_answer"
    fi
done < "$filename"
