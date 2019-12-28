#Spec file for LinPac

%define	name	linpac
%define	version	0.27
%define	release	1

%define  debug_package %{nil}

Summary: Packet Radio Terminal for Linux
Name: %{name}
Version: %{version}
Release: %{release}
Group: Applications/Communications
License: GNU General Public License (GPL)
Source: %{name}-%{version}.tar.gz
URL: http://linpac.sourceforge.net
Vendor: David Ranch (KI6ZHD) <dranch@trinnet.net>
Packager: David Ranch (KI6ZHD) <dranch@trinnet.net>
Distribution: RedHat Linux
AutoReqProv: yes
Provides: linpac
Requires: libax25 ax25apps ax25tools ncurses-libs
BuildRequires: perl
#This was required before all the vector fixes
#BuildRequires: compat-gcc-34-c++

%description
LinPac is a packet radio terminal for Linux that presents a host-mode like user interface similar to other DOS and Windows programs while keeping the TNC in KISS mode.  It allows wide configurability and easy addition of new functions and special functions needed by the user. The aim was to minimize the amount of 'hard coded' functions and create the complete set of applications that can be easy expanded and/or completely reconfigured.

New maintainer    : David Ranch (KI6ZHD) <dranch@trinnet.net>
Original author   : Radek Burget (OK2JBG) <radkovo@centrum.cz>
Project Home Page : http://linpac.sourceforge.net

%prep
rm -rf $RPM_BUILD_DIR/%{name}-%{version}

%setup
autoreconf --install

%build

#The include option does NOT work here.. only the ENV VAR approach works
./configure --prefix=/usr CXXFLAGS="-Wall -funsigned-char"

#doesnt work yet: with the vector.h fixes
#./configure --prefix=/usr

make %{?_smp_mflags}

%install
#export RPM_BUILD_ROOT="/usr/src/redhat/BUILD/%{name}-%{version}"
make install DESTDIR=$RPM_BUILD_ROOT

%files
%defattr(-,root,root)

%doc COPYING INSTALL NEWS README
#Bell must be SUID root to work with non-root users
%{_bindir}/*
%{_datadir}/*
#%{_includedir}/c++/3.4.6/linpac/*
#%{_includedir}/linpac/*
%{_includedir}/*
%{_libexecdir}/linpac/*
#Bug in linpac sending to /usr/lib and not /usr/lib64
/usr/lib/liblinpac.*
/usr/lib/libaxmail.*
#%{_docdir}/%{name}-%{version}/*
#%attr(4755 root root) /usr/share/linpac/bin/bell

%clean

%post

%postun

%changelog
* Sat Dec 28 2019 David Ranch <dranch@trinnet.net>
- New version 0.27
- Address unicode characters in messages when Linpac doesn't specifically support unicode
- Fixes for segfaults when long strings are used in the mail reader
* Sun Dec 10 2017 David Ranch <dranch@trinnet.net>
- New 0.26 version 
* Fri Sep 18 2015 David Ranch <dranch@trinnet.net>
- New 0.24 version 
* Wed Sep 16 2015 David Ranch <dranch@trinnet.net>
- New 0.23 version that adds ax25tools as a requirement for mheard support
* Sun May 24 2015 David Ranch <dranch@trinnet.net>
- New 0.22 version making the ax25mail-utils package required
* Sat Apr 04 2015 David Ranch <dranch@trinnet.net>
- New 0.21 version with some fixes from K6SPI
- Added a require libncurses requirement
* Sun Aug 03 2014 David Ranch <dranch@trinnet.net>
- New 0.20 version with all previous patches included
- Updated autoconf configuration care of Jerry Dunmire, KA6HLD
- dropped the requirement for ax25mail-utils
* Tue Sep 18 2012 David Ranch <dranch@trinnet.net>
- No longer defaulting to /usr/local path
- Added file listings into the SPEC file
* Tue Aug 14 2012 David Ranch <dranch@trinnet.net>
- Updating the spec file to include the patches, etc
