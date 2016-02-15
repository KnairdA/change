# change

...transparent filesystem change tracking using function interposition.

This experimental project consists of a library `libChangeLog` and a matching wrapper bash script named `change`. If one opens any application using `change` it automatically tracks common system calls used for manipulating filesystem contents and provides the user with a short summary including _diffs_ where appropriate.

	> change mv test example
	renamed 'test' to 'example'
	> change vim example
	--- /home/common/projects/dev/change/example
	+++ /home/common/projects/dev/change/example	2016-02-13 21:43:15.719355382 +0100
	@@ -1,3 +1,5 @@
	 1
	+
	 2
	+
	 3
	> change rm example
	removed 'example'

The goal is to develop `change` into a utility that can be dropped in front of any non-suid (function interposition via `LD_PRELOAD` is thankfully not allowed for suid-executables) application and generate a summary that will explain the actual happenings of a terminal session. While this is not very useful for simple, self-explanatory commands such as `mv $this $to_that` it is certainly helpful whenever files are changed by interactive applications that do not provide their own directly visible logging such as text editors. Such an application will in turn be useful for e.g. documenting shell sessions.

## Filtering

Due to it's nature of interposing low level system calls such as `write` and `unlink` the library by default exposes lots of the internal write logic of the wrapped application. For instance it reports _vim_ creating a file called `4913` to verify the target directory's writability as well as the creation of various temporary backup files. While this is certainly interesting for debugging purposes it hinders the library's goal of providing a higher level summary consisting primarily of the actions the user explicity performed such as the changed file contents.

To solve this problem one may provide a list of regular expressions to be matched against the file paths via the `CHANGE_LOG_IGNORE_PATTERN_PATH` environment variable.

For example the following ruleset intructs the library to restrict the output `change vim $file` to a _diff_ of all files changed by the wrapped application:

	# vim's way of verifying that it is able to create a file
	[0-9]+
	# temporary backup file during write
	[^~]*~
	# log and backup files
	.*\.viminfo
	.*\.sw[px]

## Build

	mkdir build
	cd build
	cmake ..
	make
	make install

Note that this project depends on `boost::filesystem` as well as `boost::process` in addition to a current C++14 compiler.
