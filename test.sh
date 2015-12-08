mm
adb push /home/ggong/develop/aosp/loliipop/out/target/product/hammerhead/symbols/system/bin/expomx /data/local/tmp/expomx
if [ $# == 0 ]; then
    adb shell  setenforce 0
    adb shell  /data/local/tmp/expomx 
else
    adb shell su -c setenforce 0
    adb shell su -c gdbserver :6666 /data/local/tmp/expomx tt &
    adb forward tcp:5039 tcp:5039
    adb forward tcp:6666 tcp:6666
    gdbclient expomx :6666
fi
