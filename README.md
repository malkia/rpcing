playing with grpc, bazel, and multiple languages
on multiple platforms (windows, linux, etc)

```
malkia@penguin:~/p/rpcing$ time -p bazel run :ken -c opt
INFO: Invocation ID: 0cfeac99-d1fa-4a8f-a1b7-08b90b548528
DEBUG: /home/malkia/p/rpcing/lizard.bzl:2:5: grub
INFO: Analysed target //:ken (0 packages loaded, 0 targets configured).
INFO: Found 1 target...
Target //:ken up-to-date:
  bazel-bin/linux_amd64_stripped/ken
INFO: Elapsed time: 1.416s, Critical Path: 1.08s
INFO: 2 processes: 2 linux-sandbox.
INFO: Build completed successfully, 3 total actions
INFO: Build completed successfully, 3 total actions
hello world %+v {12 {} [] 9}
hello world %+v {0 {} [] 0}
hello world %+v {12 {} [] 0}
a=%+v <nil>
b=%+v <nil>
2019/02/16 19:00:48 Received: command_id:12 
real 6.09
user 12.39
sys 4.27
```