#!/bin/bash

# necessary parametrs
logDisk=$1
backupDisk=$2

if [[ ! -d "$logDisk" ]]; then
    echo "[ERROR]: Set true path to the disk with /log as first parametr."
    exit 1
fi

if [[ ! -d "$backupDisk" ]]; then
    echo "[ERROR]: Set true path to the disk with /backup as second parametr."
fi

# other parametrs
X=${3:-70}
N=${4:-5}

if [[ $X -lt 0 || $X -gt 100 ]]; then 
    echo "[ERROR]: Set true parcentage X as third parametr."
    exit
fi

if [[ $N -lt 0 ]]; then 
    echo "[ERROR]: Set true count of compressiong files N as 4th parametr."
    exit
fi

diskUsage=$(df "$logDisk" | awk 'NR==2 {gsub("%",""); print $5}')
filesCount=$(find $logDisk -type f | wc -l)

echo "[INFO]: diskUsage = ${diskUsage}, filesCount = ${filesCount}, X = ${X}, N = ${N}"

if [[ $diskUsage -lt $X ]]; then
    echo "Compression is not demanded. Complete!"
elif [[ $filesCount -ge $N ]]; then
    mapfile -t files < <(ls -t "$logDisk" | head -n "$N")
    
    time=$(date +%Y-%m-%d_%H-%M-%S)
    archiveName="backup_$time.tar.gz"
    echo -n "[INFO]: Files: "
    for file in "${files[@]}"; do
        echo -n "$file "
    done
    echo "."

    echo -n "Files are compressing..."

    tar -czf "${backupDisk}/${archiveName}" -C "${logDisk}" "${files[@]}"

    echo -n "Old files are deleting..."

    for file in "${files[@]}"; do
        rm -f "${logDisk}/${file}"
    done

    echo -e "\rFiles were deleted and compressed in ${backupDisk}/${archiveName}. Complete!"
    
else 
    echo "There are too few files in the directory. Need $N files for compression."
fi
