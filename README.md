## Advanced Distributed System(ADS) TFS demo

### Paper

https://ipads.se.sjtu.edu.cn/courses/ads/paper/ads-tfs.pdf


### Setup

```
make clean && make all
```

You can use `make init` to initialize TFS.

### Usage

Normal file operations:

```
welcome to tfs!
tfs-local > help
quit/q
create/touch [-t] filename
remove/rm [-f/-r] filename/dirname
mkdir dirname
ls
cd dirname
pwd
cat filename
echo "content" filename
lsblk
open filename
close filename
write "content" filename
read filename
```

Example:

```
❯ ./tfs_local 
welcome to tfs!
tfs-local > ls
. .. 
tfs-local > touch a.txt
tfs-local > mkdir dir
tfs-local > ls
. .. a.txt dir 
tfs-local > echo 123 a.txt
tfs-local > cat a.txt
123
tfs-local > cd dir
tfs-local > ls
. .. 
tfs-local > touch b.txt
tfs-local > ls
. .. b.txt 
tfs-local > cd ..
tfs-local > ls
. .. a.txt dir 
tfs-local > q
```

**TFS related**:

```
❯ ./tfs_local
welcome to tfs!
tfs-local > touch -t a.txt
tfs-local > echo 1234 a.txt
tfs-local > lsblk

❯ ./tfs_remote
welcome to tfs!
tfs-remote > ls
a.txt 
tfs-remote > cat a.txt
1234
tfs-remote > 
```

You can create and write both normal files and transparent files at the same time, and use `lsblk` to show the block state transition. 

Note that in tfs_remote, both `touch and `touch -t` create transparent files. 

If tfs_remote is opening a file(of course it's a transparent file) use `open` operation, tfs_local can not overwritten its block unless it was closed by tfs_remote.
