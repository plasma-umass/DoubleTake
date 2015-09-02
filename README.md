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

## Note

In order to run certain applications, the maximum number of open files must be increased
(this includes, for example, the reverse_index benchmark).

To do this for a specific user, see
http://askubuntu.com/questions/162229/how-do-i-increase-the-open-files-limit-for-a-non-root-user

## License

All source code is licensed under the MIT license.
Copyright (C) 2014-2015 University of Massachusetts Amherst

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

