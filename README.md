# change

…transparent filesystem change tracking using function interposition.

This experimental project consists of a library `libChangeLog` and a matching wrapper bash script named `change`. If one opens any application using `change` it automatically tracks common system calls used for manipulating filesystem contents and provides the user with a short summary including _diffs_ where appropriate.

	λ ~/p/d/change ‣ change mv test example
	renamed 'test' to 'example'
	λ ~/p/d/change ‣ change vim example
	removed '5405'
	renamed 'example' to 'example~'
	removed 'example~'
	removed '/home/common/.viminfo'
	renamed '/home/common/.viminfk.tmp' to '/home/common/.viminfo'
	removed '/home/common/.vim/swap//%home%common%projects%dev%change%example.swp'
	--- /home/common/projects/dev/change/example
	+++ /home/common/projects/dev/change/example	2016-02-13 21:43:15.719355382 +0100
	@@ -1,3 +1,5 @@
	 1
	+
	 2
	+
	 3
	λ ~/p/d/change ‣ change rm example
	removed 'example'

As we can see the library currently still exposes some of the internal doings of e.g. _vim_ that we don't really want to see in our usecase. The goal is to develop `change` into a utility that can be dropped in front of any non-suid (function interposition via `LD_PRELOAD` is thankfully not allowed for suid-executables) shell command and generate a summary that will explain the actual happenings of a terminal session. While this is not very useful for simple, self-explanatory commands such as `mv $this $to_that` it is certainly helpful whenever files are changed by interactive applications that do not provide their own directly visible logging such as text editors. Such an application will in turn be useful for e.g. documenting shell sessions.

## Build

	mkdir build
	cd build
	cmake ..
	make
	make install

Note that this project depends on `boost::filesystem` as well as `boost::process` in addition to a current C++14 compiler.
