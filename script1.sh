#! /bin/bash
#Написать скрипт с использованием цикла for, выводящий на консоль размеры и права доступа для всех файлов 
#в заданном каталоге и всех его подкаталогах (имя каталога задается пользователем
#в качестве первого аргумента командной строки).
#На консоль выводится общее число просмотренных файлов.
IFS=$'\n'
for i in $(find "$1" -type f)
do      
      gfind $i -type f  -printf "%p %s %m \n" 2>/tmp/err.txt
      gsed "s/gfind/`basename $0`/" /tmp/err.txt>&2

        
done