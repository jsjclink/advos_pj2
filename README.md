# TinyFile Service
903973230 Sungjae Chung 903973231 Sangwoo Kim

## Instructions

### Build + Service Start
* ./tfservice.sh
*  make clean
### Client Application(On separate bash shell)
* ./bin/usapp3 --file ./input/Huge.jpg --state SYNC

* ./bin/usapp3 --file ./input/Huge.jpg --state ASYNC

* ./bin/usapp3 --files ./input/file_list.txt --state SYNC

* ./bin/usapp3 --files ./input/file_list.txt --state ASYNC

Typically will use client applications with >output.txt because sometimes printing the compressed file acts strangely.  
