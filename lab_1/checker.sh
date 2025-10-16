#!/bin/bash

logDisk=$1
backupDisk=$2
X=${3:-70}
N=${4:-0}

if [[ ! -d "$logDisk" ]]; then
    echo "[ERROR]: Set true path to the disk with /log as first parameter."
    exit 1
fi

if [[ ! -d "$backupDisk" ]]; then
    echo "[ERROR]: Set true path to the disk with /backup as second parameter."
    exit 1
fi

if [[ $X -lt 0 || $X -gt 100 ]]; then 
    echo "[ERROR]: Set true percentage X as third parameter."
    exit 1
fi

if [[ $N -lt 0 ]]; then 
    echo "[ERROR]: Set true count of compressing files N as 4th parameter."
    exit 1
fi

diskUsage=$(df "$logDisk" | awk 'NR==2 {gsub("%",""); print $5}')
diskName=$(df "$logDisk" | awk 'NR==2 {print $1}')

echo "[INFO]: Disk: ${diskName}"
echo "[INFO]: Disk space used: ${diskUsage}%"

if [[ $diskUsage -lt $X ]]; then
    echo "[INFO]: Compression is not demanded. Complete!"
    exit 0
fi

files=()
while IFS= read -r file; do
    files+=("$file")
done < <(find "$logDisk" -maxdepth 1 -type f -name "*.log" -printf "%T@ %p\n" | sort -n | cut -d' ' -f2-)

if [[ ${#files[@]} -eq 0 ]]; then
    echo "[INFO]: No log files found. Complete!"
    exit 0
fi

if [[ $N -gt 0 ]]; then
    files=("${files[@]:0:${#files[@]}-N}")
    if [[ ${#files[@]} -eq 0 ]]; then
        echo "[INFO]: After preserving $N newest files, nothing to archive. Complete!"
        exit 0
    fi
fi

archive="$backupDisk/archive-$(date +%Y%m%d%H%M%S).tar.gz"

# echo "[INFO]: Archiving ${#files[@]} files:"
# for f in "${files[@]}"; do
#     echo "  → $(basename "$f")"
# done

if tar -czf "$archive" -C "$logDisk" "${files[@]##*/}"; then
    rm "${files[@]}"
    echo "[INFO]: Archived ${#files[@]} files to $(basename "$archive")"
else
    echo "[ERROR]: Archiving failed!"
    rm -f "$archive"
    exit 1
fi

echo "[INFO]: Done."
