#!/bin/bash
## Script to replace angle brackets with quotes+path in all .h and .cpp files in a directory. This is necessary to make the import happen from the local folder, not the global libraries folder, which is required for portability.

## run this from the src/ directory. The input is the library directory. It assumes that all the .h and .cpp files are in the library root directory.
dir=$1
for file in $dir/*{h,cpp}
do
    echo $dir
    dir2=$(echo $dir | sed 's/\//\\\//g') # this is preferred to backticks
    echo $dir2
    echo $file
    ## substitute the left bracket for quote and path
    #sed -i "s/include </include \"src\/$dir2/g" $file # this is for when the path needs to be from the Gem root dir, not needed here
    sed -i "s/include </include \"/g" $file # When, for example, SdFat.h #includes SdFile.h, they are in the same directory so there's no path.
    ## substitute the right bracket for quote
    sed -i 's/.h>/.h"/g' $file
done


##################################
## look for #includes in files in subdirectories that don't have good paths
## both of these should return #include statements with '..' in them
#for i in */*.h; do grep $i */*{h,cpp} | grep '#include'; done
#for i in *.h; do grep $i */*{h,cpp} | grep '#include'; done
#if((file->fileSize() > 0) && (current_fn > greatest_fn))
