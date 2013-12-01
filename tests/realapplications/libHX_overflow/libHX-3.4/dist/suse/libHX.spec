
Name:		libHX22
%define lname	libHX
Version:	3.4
Release:	0
Group:		System/Libraries
URL:		http://libhx.sf.net/
Summary:	Useful collection of routines for C and C++ programming
License:	LGPL2+
Source:		http://downloads.sf.net/libhx/libHX-%version.tar.xz
BuildRoot:	%_tmppath/%name-%version-build
BuildRequires:	gcc-c++ xz
# no, libxml2-devel is NOT required because nothing
# that requires it is going to be compiled.
# gcc-c++ is pretty optional and only used for make check

%description
A library for:
- hash/rbtree-based maps/sets
- double-ended queues (stacks/fifos/lists)
- platform-independent opendir-style directory access
- platform-independent dlopen-style shared library access
- auto-storage strings with direct access
- command line option (argv) parser
- shconfig-style config file parser
- various string, memory and zvec ops
- more

%package -n libHX-devel
Group:		Development/Libraries/C and C++
Summary:	Useful collection of routines for C and C++ programming
Requires:	%name = %version

%description -n libHX-devel
A library for:
- hash/rbtree-based maps/sets
- double-ended queues (stacks/fifos/lists)
- platform-independent opendir-style directory access
- platform-independent dlopen-style shared library access
- auto-storage strings with direct access
- command line option (argv) parser
- shconfig-style config file parser
- various string, memory and zvec ops
- more

%prep
%setup -n %lname-%version

%build
if [ ! -e configure ]; then
	./autogen.sh
fi;
%configure
make %{?_smp_mflags}

%install
b="%buildroot"
rm -Rf "$b"
mkdir "$b"
make install DESTDIR="$b" docdir="%_docdir/%lname"
rm -f "$b/%_libdir/%lname.la"
mkdir -p "$b/%_docdir/%lname"
install -pm0644 doc/*.txt "$b/%_docdir/%lname/"

%check
make check

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root)
%_libdir/%{lname}*.so.*

%files -n libHX-devel
%defattr(-,root,root)
%_libdir/%{lname}*.so
%_libdir/pkgconfig/*
%_includedir/*
%doc %_docdir/%lname

%changelog
