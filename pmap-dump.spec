Name:		pmap-dump
Version:	0.92
Release:	1%{?dist}
Summary:	Dump running process memory segments into files

Group:		Applications/System
License:	GPL v.3
URL:		https://github.com/Nopius/pmap-dump
Source0:	https://github.com/Nopius/pmap-dump/archive/0.92.zip

BuildRequires:	gcc make

%description
Utility to dump memory from virtual address of the running process to a file.

%prep
%setup -q

%build
make %{?_smp_mflags}

%install
make install DESTDIR=%{buildroot}


%files
/usr/sbin/pmap-dump

%doc

%changelog
* Tue Jul 23 2019 nopius@nopius.com
- version 0.92
  * Added help message
  * Makefile is GNU friendly

* Mon Jul 22 2019 nopius@nopius.com
- Initial version 0.9
  * not well tested, but working
