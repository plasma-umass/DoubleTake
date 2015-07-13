# DoubleTake

A fast checker for memory errors.

DoubleTake can pinpoint memory errors in C/C++ code, including heap
buffer overflows, use-after-free errors, and memory leaks. DoubleTake
is *fast*. When detecting buffer overflows only, it imposes just 2%
overhead on average.

Unlike other tools that examine state as the program runs and
therefore impose considerable slowdowns, DoubleTake uses a new
*evidence-based dynamic analysis* approach. DoubleTake lets
applications run at full speed until it discovers evidence that an
error occurred at some time in the past. Since the common case is that
there is no error, this makes it very fast. If it does find an error,
DoubleTake rolls execution back and replays with instrumentation in
place to locate the exact place where the error happened. This
approach lets DoubleTake achieve high precision (no false positives)
with the lowest overhead to date of any dynamic analysis approach.

## License

All source code is licensed under the GPLv2 unless otherwise indicated.
Copyright (C) 2014 University of Massachusetts Amherst

## Note:
Please increase the limit for number of files in order to run reverse_index or other applications. 
In order to create the exactly the same sequence, we will preserve the order of file open/close. 
Thus, DoubleTake won't close those files until the checking of epoch. 
To increase the number of files for a specific user, we can change like http://askubuntu.com/questions/162229/how-do-i-increase-the-open-files-limit-for-a-non-root-user
