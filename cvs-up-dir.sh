#!/bin/bash


if [ $# == 1 ]
    then
    comments=$1
else
    comments=`whoami` 		# default 
fi


function do_dir()
{
    for child in `ls`   # ommit emacs's tmp files 
      do
      if [ -f $child ]
	  then
	  echo "cvs add "$child
	  cvs add $child
	  echo "cvs ci -m \"$comments\" "$child
	  cvs commit -m \"$comments\" $child
	  echo "-------------"
      elif [ -d $child ]
	  then
	  echo "cvs add dir:"$child
	  #cvs add $child
	  cd $child
	  do_dir      		# do child dir again
	  cd .. # back to last dir
      fi
    done
}


### main function ###

#cd $TOPDIR


echo "Your CVS up directory is "
pwd
echo "Are you sure? [yes] or [no]"
read COMMIT


if [ $COMMIT != "yes" ]
    then
    echo "ERR, bye..."
    exit
fi


do_dir



