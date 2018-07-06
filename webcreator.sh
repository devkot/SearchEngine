#!/bin/bash
declare -A matrix

# Checking arguments are ok
if [ ! -d $1 ];then
	echo "Root Directory not found!Exiting..."
	exit 1
fi

if [ ! -f $2 ];then
	echo "Text File not found!Exiting..."
	exit 2
fi

re="^[0-9]+$"
if ! [[ $3 =~ $re ]];then
	echo "W is not an integer!Exiting..."
	exit 3
fi

if ! [[ $4 =~ $re ]];then
	echo "P is not an integer!Exiting..."
	exit 4
fi

nlines=$(wc -l $2 | cut -d' ' -f1)
if [[ $nlines -lt 10000 ]];then
	echo "Text File has less then 10000 words!Exiting..."
	exit 5
fi

# Checking the Root directory is empty

files=""
files=$(find $1 -depth -type f -name "*.*")
if [[ ! $files -eq "" ]];then
	echo "Warning: Directory is full!Purging..."
	rm -rf $1 # CAUTIOOOOOOOOOOOOON!!!! :)
fi

# Populate a 2D array with random links
for ((row=0;row<$3;row++)) do
	for ((col=0;col<$4;col++)) do
		matrix[$row,$col]=$(( $RANDOM % 9999 ))
	done
done

# Iterate w times to create w sites
for ((i=0;i < $3;i++)); do
	
	printf "Creating web site %d...\n" $i
	mkdir -p $1'/site'$i'/'

	f=$(( $4/2 + 1 ))
	q=$(( $3/2 + 1 ))
	#i=0

	# Iterate p times to create p pages
	for ((i2=0;i2 < $4;i2++)); do
		k=$(( ($RANDOM % ($nlines - 2000)) + 1))
		m=$(( ($RANDOM % 1000) + 1000))
	
		pageno=${matrix[$i,$i2]}
		filename=$1'/site'$i'/page'$i2'_'$pageno'.html'
		echo $filename >> ./allPages.txt
		printf "\tCreating page %s with %d lines starting at line %d...\n" $filename $m $k
		echo "<!DOCTYPE html>" > $filename
		echo "<html>" >> $filename
		echo "<body>" >> $filename

		fi=0
		qi=0
		lineno=0
		while IFS='' read -r line || [[ -n "$line" ]];do
			lineno=$((lineno+1))
	    	if [[ $lineno -lt $k ]]; then
	    		continue
	    	elif [[ $lineno -eq $k ]];then
	    		j=0
	    	fi

	    	if  [[ $(($lineno % ($m/($f+$q)))) -ne 0 ]]; then
	    		echo $line >> $filename
	    	else
	    		if [[ $fi -lt $f ]];then
	    			randompage=$i2

	    			while [[ $randompage -eq $i2 ]]; do 	# skip itself
	    				randompage=$(( $RANDOM % $4 ))
	    			done

					linknumber=${matrix[$i,$randompage]}	# i represents internal link, second argument is random
	    			internallink=$1'/site'$i'/page'$randompage'_'$linknumber'.html'
	    			
    				echo "	Adding link to $1/site${i}/page${randompage}_${linknumber}.html"
    				echo "<a href=\"../site$i/page${randompage}_${linknumber}.html\">internal url ${fi}</a>" >> $filename
    				echo "$1/site${i}/page${randompage}_${linknumber}.html" >> ./uniqueLinks.txt
	    	
	    			fi=$((fi+1))
	    			if [[ $fi -eq $f ]]; then
	    				echo "..."
	    			fi
	    		elif [[ $qi -lt $q ]];then
					randomcol=$(($RANDOM % $4))
					randomsite=$i

					while [[ $randomsite -eq $i ]]; do 		# skip itself
						randomsite=$(( $RANDOM % $3 ))
					done

					randomlink=${matrix[$randomsite,$randomcol]}	# get a random array position

	    			echo "	Adding link to $1/site${randomsite}/page${randomcol}_${randomlink}.html"
	    			echo "<a href=\"../site${randomsite}/page${randomcol}_${randomlink}.html\">external url ${qi}</a>" >> $filename
	    			echo "$1/site${randomsite}/page${randomcol}_${randomlink}.html" >> ./uniqueLinks.txt
	    			qi=$((qi+1))
	    			if [[ $qi -eq $q ]]; then
	    				echo "..."
	    			fi
	    		else
	    			break
	    		fi
	    	fi
		done < "$2" 
		echo "</body>" >> $filename
		echo "</html>" >> $filename
	done
done

cat ./uniqueLinks.txt | sort | uniq > ./orderedLinks.txt
cat ./allPages.txt | sort | uniq > ./orderedPages.txt

temp1='./orderedLinks.txt'
temp2='./orderedPages.txt'

if diff $temp1 $temp2 >/dev/null ; then
	echo "All pages have at least one incoming link."
else
	echo "Not all pages have at least one incoming link."
fi

rm -f ./orderedPages.txt
rm -f ./orderedLinks.txt
rm -f ./allPages.txt
rm -f ./uniqueLinks.txt

echo "Done."