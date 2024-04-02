## ADS TFS demo

### Setup

```
make all
```

### Example

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

