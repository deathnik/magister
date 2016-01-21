filename=$1

welcome="Type the phrases you hear. Type a single question mark character to repeat, empty line to finish"
#festival -b "(begin  (SayText \"$welcome\" nil))" ;
l=$(shuf -n 1 $filename)
echo $l

while IFS='' read -r line || [[ -n "$line" ]]; do
	while true ; do
		festival -b "(begin  (SayText \"$line\" nil))" ;
       	read answer </dev/tty
       	if [ " $answer" != " ?" ];
       	then
       		break;
       	fi
    done

    if [ " $answer" == " " ];
    then
    	break;
    fi
done < "$filename"
