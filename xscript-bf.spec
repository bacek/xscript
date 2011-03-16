Summary:       XScript is xml-based application server written in C++
Name:          xscript
Version:       5.73.20
Release:       1%{?dist}
Group:         System Environment/Libraries
License:       GPLv2
Source:        %{name}-%{version}.tar.bz2
BuildRoot:     %{_tmppath}/%{name}-%{version}-%{release}-root

BuildRequires: automake
BuildRequires: pkgconfig
BuildRequires: libtool
BuildRequires: gettext-devel
BuildRequires: autoconf
BuildRequires: boost-devel
BuildRequires: python-devel
BuildRequires: curl-devel
BuildRequires: fcgi-devel
BuildRequires: cppunit-devel
BuildRequires: libxml2-devel
BuildRequires: libxslt-devel
BuildRequires: openssl-devel
BuildRequires: lua-devel
BuildRequires: memcached
BuildRequires: autoconf-extra-archive

%package       devel
Summary:       XScript is xml-based application server written in C++
Group:         System Environment/Libraries
Requires:      %{name} = %{version}-%{release}
Requires:      boost-devel
Requires:      curl-devel
Requires:      fcgi-devel
Requires:      libxml2-devel
Requires:      libxslt-devel
Requires:      openssl-devel

%package       standard
Summary:       XScript is xml-based application server written in C++
Group:         System Environment/Libraries
Requires:      %{name} = %{version}-%{release}

%package       memcached
Summary:       XScript is xml-based application server written in C++
Group:         System Environment/Libraries
Requires:      %{name} = %{version}-%{release}

%package       daemon
Summary:       XScript is xml-based application server written in C++
Group:         Networking/Daemons/FastCGI
Requires:      %{name} = %{version}-%{release}

%package       utility
Summary:       XScript is xml-based application server written in C++
Group:         Networking/Daemons/FastCGI
Requires:      %{name} = %{version}-%{release}

%package       utility-devel
Summary:       XScript is xml-based application server written in C++
Group:         System Environment/Libraries
Requires:      %{name} = %{version}-%{release}

%package       python
Summary:       XScript is xml-based application server written in C++
Group:         System Environment/Libraries
Requires:      %{name} = %{version}-%{release}

%description
The core package

%description   devel
Headers and libraries to develop XScript extensions

%description   standard
Standard modules such as logger, thread pool and various blocks

%description   memcached
Module to work with distributed cache system.

%description   daemon
Binary that runs as factcgi-daemon

%description   utility
XScript offline processor

%description   utility-devel
Headers and libraries to develop oflline XScript extensions

%description   python
Python binding to XScript

%prep
%setup -q

%build
ACLOCAL_OPTIONS="-I config" ./autogen.sh
%configure --prefix=/usr/ --sysconfdir=/etc/xscript --enable-cppunit
make %{?_smp_mflags}

%install
%{__rm} -rf %{buildroot}
%{__make} install DESTDIR=%{buildroot}
mkdir -p %{buildroot}/usr/bin
install -m 755 extra/xscriptstart.sh %{buildroot}/usr/bin
install -m 755 extra/xscriptcacheclean.sh %{buildroot}/usr/bin
mkdir -p %{buildroot}/usr/local/bin
mkdir -p %{buildroot}/etc/cron.d
install -m 644 extra/xscript-cache-clean %{buildroot}/etc/cron.d
rm -f %{buildroot}/%{_libdir}/xscript/xscript-*.la
rm -f %{buildroot}/%{_libdir}/xscript/xscript-*.a
rm -f %{buildroot}/usr/local/bin/xscript-fix-shlibdeps.sh

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-, root, root)
%doc README
%{_libdir}/libxscript.so.*
/etc/xscript/xscript.conf.example

%files devel
%defattr(-, root, root)
%doc README
%{_libdir}/libxscript.a
%{_libdir}/libxscript.la
%{_libdir}/libxscript.so
%{_includedir}/xscript/*.h

%files standard
%defattr(-, root, root)
%doc README
%{_libdir}/xscript/xscript-d*so*
%{_libdir}/xscript/xscript-f*so*
%{_libdir}/xscript/xscript-h*so*
%{_libdir}/xscript/xscript-l*so*
%{_libdir}/xscript/xscript-s*so*
%{_libdir}/xscript/xscript-t*so*
%{_libdir}/xscript/xscript-v*so*
%{_libdir}/xscript/xscript-x*so*
%{_libdir}/xscript/xscript-mist.so*
%{_libdir}/xscript/xscript-memcache.so*
%{_bindir}/xscriptcacheclean.sh
/etc/cron.d/xscript-cache-clean

%files memcached
%defattr(-, root, root)
%doc README
%{_libdir}/xscript/xscript-memcached*so*

%files daemon
%defattr(-, root, root)
%doc README
%{_bindir}/xscript-bin
%{_bindir}/xscriptstart.sh
/etc/xscript/xscript-ulimits.conf

%files utility
%defattr(-, root, root)
%doc README
%{_bindir}/xscript-proc
/etc/xscript/offline.conf
%{_datadir}/xscript-proc/profile.xsl
%{_libdir}/libxscript-offline.so.*

%files utility-devel
%defattr(-, root, root)
%doc README
%{_libdir}/libxscript-offline.a
%{_libdir}/libxscript-offline.la
%{_libdir}/libxscript-offline.so
%{_includedir}/xscript-utility/*

%files python
%defattr(-, root, root)
%doc README
%{_libdir}/python*/*-packages/*

%changelog
* Tue Oct 27 2009 Leonid A. Movsesjan <lmovsesjan@yandex-team.ru>
- initial rpm build

