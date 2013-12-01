───────────────────────────────────────────────────────────────────────────────

libHX

A general-purpose library for everyday coding tasks.

  • maps (HXmap_*)

    Originally created to provide a data structure like Perl's associative
    arrays. Different map types and characteristics are available, such as
    hash-based or traditional rbtree.

  • deques (HXdeque_*, HXlist_*, HXclist_*)

    Deques — double-ended queues, essentially a doubly-linked list — are
    suitable for both providing stack and queue functionality.

  • directory handling (HXdir_*)

    HXdir provides for opendir-readdir-closedir semantics. Windows uses a
    different kind, so it had to be naturally covered up. On the other hand,
    Solaris's readdir() implementation is nasty in terms of memory management.
    HXdir covers up these discrepancies and provides a sane Linux-style
    readdir.

    Convenience functions mkdir (create all missing parents), rrmdir (rm -Rf).

  • string formatter with placeholders (HXformat_*)

    HXformat is something in the direction of printf(), but the argument list
    is not implemented by means of varargs, so is flexible even beyond compile
    time. You can change the format string — in fact, just let the user
    configuration provide it — without having to worry about argument
    evaluation problems. Positional and optional arguments are simply free.

  • memory containers, auto-sizing string ops (HXmc_*)

    At the cost of slightly increased number memory allocations as you work
    with the buffers, the hmc collection of functions provide scripting-level
    semantics for strings. Appending to a string is simply hmc_strcat(&apm;s,
    "123") [cf. $s .= "123", without having to worry about overflowing a
    buffer.

  • option parsing (HXoption_*)

    Put blunt, libpopt failed to do some elementary things and there was no
    maintainer to fix it. Well, it's packaged with rpm which already diverged
    in all distros.

  • shellconfig parser (HXshconfig_*)

    Parsers shconfig files. Their format is a subset of shell code. Files in /
    etc/sysconfig are commonly shconfig-style.

  • common string operations

    basename, chomp, dirname, getl(ine), split, strlower/-upper, str*trim,
    strsep, strsep2, etc.

Resources

  • libHX GIT repository (gitweb) — http://libhx.git.sf.net/
  • libHX clone URL — git://libhx.git.sf.net/gitroot/libhx/libhx
