## Advanced Distributed System(ADS) TFS demo

### Paper

https://ipads.se.sjtu.edu.cn/courses/ads/paper/ads-tfs.pdf


### Setup

```
make all
```

### Example

Normal file operations:

```
$ ./main
welcome to tfs-demo!
tfs-demo > ls
. .. 
tfs-demo > touch a.txt
tfs-demo > mkdir dir
tfs-demo > ls
. .. a.txt dir 
tfs-demo > echo 123 a.txt
tfs-demo > ls
. .. a.txt dir 
tfs-demo > cat a.txt
123
tfs-demo > cd dir
tfs-demo > ls
. .. 
tfs-demo > touch b.txt
tfs-demo > ls
. .. b.txt 
tfs-demo > cd ..
tfs-demo > ls
. .. a.txt dir 
tfs-demo > q 
[1] tfs-demo finish!
```

TFS related:

```
$ ./main
welcome to tfs-demo!
tfs-demo > touch -t a.txt  // use "-t" option to create a transparent file
tfs-demo > echo 1234 a.txt // write some content to transparent file
tfs-demo > lsblk           // use "lsblk" to show current data blocks (16 blocks on default)
```

You can create and write both normal files and transparent files at the same time, and use `lsblk` to show the block state transition.
