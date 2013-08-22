#
# spec file for willabweather plasmoid.
#
Summary: A Simple weather plasmoid for KDE.
Name: willabweather
Version: 1.04
Release: 1
Copyright: GPL
Group: Plugins
Source: plasma-willabweather-1.04.tar.gz
URL: http://kde-apps.org/content/show.php?content=149420
Packager: Jari Tervonen <jjtervonen@gmail.com>

%description
A simple plasmoid which displays the weather at VTT's weather station in Oulu (Finland).

%prep
rm -rf $RPM_BUILD_DIR/plasma-willabweather-1.04/build
zcat $RPM_SOURCE_DIR/plasma-willabweather-1.04.tar.gz | tar -xvf -

%build
./install

%install
cd build
make install

%files
%doc README
/usr/lib/kde4/plasma_applet_willabweather.so
/usr/share/kde4/services/plasma-applet-willabweather.desktop
