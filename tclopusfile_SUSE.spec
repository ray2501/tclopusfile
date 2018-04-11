%{!?directory:%define directory /usr}

%define buildroot %{_tmppath}/%{name}

Name:          tclopusfile
Summary:       Tcl bindings for Opusfile library
Version:       0.2
Release:       0
License:       MIT
Group:         Development/Libraries/Tcl
Source:        %{name}-%{version}.tar.gz
URL:           https://github.com/ray2501/tclopusfile
BuildRequires: autoconf
BuildRequires: make
BuildRequires: tcl-devel >= 8.4
BuildRequires: opusfile-devel
Requires:      tcl >= 8.4
Requires:      libopusfile0
BuildRoot:     %{buildroot}

%description
Tcl bindings for Opusfile library.

%prep
%setup -q -n %{name}-%{version}

%build
./configure \
	--prefix=%{directory} \
	--exec-prefix=%{directory} \
	--libdir=%{directory}/%{_lib}
make 

%install
make DESTDIR=%{buildroot} pkglibdir=%{tcl_archdir}/%{name}%{version} install

%clean
rm -rf %buildroot

%files
%defattr(-,root,root)
%{tcl_archdir}
