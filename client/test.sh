for((j=0;j<1000000;j++))
do
    MAX=2048
    for((i=0;i<MAX;i++))
    do
        echo $j,$i
        ./IdcreatorClient 127.0.0.1 9048  $i
    done
done
