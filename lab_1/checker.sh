#!/bin/bash

currentPath=$1
X=${2:-70}
N=${3:-5}

if [[ $currentPath == "" ]]; then
    echo "Не указан путь до директории"
    exit
fi

if [[ $X -lt 0 || $X -gt 100 ]]; then 
    echo "Укажите корректный X%"
    exit
fi

filesCount=$(echo find $currentPath -type f | wc -l)


echo $currentPath
echo $X
echo $N
echo $filesCount
