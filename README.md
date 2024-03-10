### Argument Synopsis

* The service program/application should accept the configuration filepath/arguments as a runtime parameter.
  
  * Under directory ``bin/input``, there are sample files for compression.

* TinyFile service

  * Working mode should be a configurable runtime parameter.
  * parameters:
    * ``--n_sms``: # shared memory segments
    * ``--sms_size``: the size of shared memory segments, in the unit of bytes.
    * e.g. running TinyFile service in blocking mode: `./tinyfile --n_sms 5 --sms_size 32`

* A sample Application to use the service using TinyFile Library

  * The number and size of the shared memory segments should be as configurable runtime parameters.
  * The path to indicate the file(s) to be compressed
  
  * parameters:
    * ``--state``: SYNC | ASYNC
    * ``--file``: specify the file path to be compressed
    * ``--files``: specify the file containing the list of files to compressed. You will want to take advantage of this argument to test QoS.
    
    * For example, if you were to run a sample application: 
    
      * for single file request: `./sample_app --file ./aos_is_fun.txt --state SYNC`
    
      * for multi-file request: `./sample_app --files ./filelist.txt --state SYNC`
    
      * the files put in the filelist file should be listed in a column:
    
        ```
        $ cat filelist.txt
        input/Large.txt
        input/Huge.jpg
        :
        ```
    
  * You can possibly add other arguments as you need, just make sure putting some note in your report. Also, add `README` file with instruction.
  
### How to Use
* make lib
* make && make app
* ./bin/service
* ./bin/usapp
* make clean