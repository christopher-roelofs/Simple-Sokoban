#
# spec file for package simplesok
#
# Copyright (c) 2014-2022 Mateusz Viste
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.
#
# Please submit bugfixes or comments via http://bugs.opensuse.org/
#

Name: simplesok
Version: 1.0.2
Release: 1%{?dist}
Summary: Simple Sokoban is a colorful Sokoban game aimed at playability and portability
Vendor: Mateusz Viste

Group: Amusements/Games/Logic

License: MIT
URL: http://simplesok.sourceforge.net/
Source0: %{name}-%{version}.tar.xz

%if 0%{?fedora}
BuildRequires: SDL2-devel >= 2.0.1
%else
BuildRequires: libSDL2-devel >= 2.0.1
%endif
BuildRequires: zlib-devel

%description
Simple Sokoban is a colorful Sokoban game aimed for playability and portability across systems. It is written in ANSI C89, using SDL2 for user interactions.

# this one is required so Fedora build does not fail
%global debug_package %{nil}

%prep
%setup

%build
make

%check

%install
install -D simplesok %buildroot/%{_bindir}/simplesok
mkdir -p %buildroot/usr/share/icons/hicolor/48x48/apps/
install -D simplesok.png %buildroot/usr/share/icons/hicolor/48x48/apps/
mkdir -p %buildroot/usr/share/simplesok/skins/
install -D skins/*.bmp.gz %buildroot/usr/share/simplesok/skins/
install -D simplesok.6 %buildroot/%{_mandir}/man6/simplesok.6

%files
%dir /usr/share/icons/hicolor/
%dir /usr/share/icons/hicolor/48x48/
%dir /usr/share/icons/hicolor/48x48/apps
%dir /usr/share/simplesok/
%dir /usr/share/simplesok/skins/
%attr(644, root, root) %doc simplesok.txt history.txt xsb_format.txt
%attr(755, root, root) %{_bindir}/simplesok
%attr(644, root, root) /usr/share/icons/hicolor/48x48/apps/simplesok.png
%attr(644, root, root) /usr/share/simplesok/skins/*.bmp.gz
%attr(644, root, root) %{_mandir}/man6/simplesok.6*

%changelog
* Thu Dec 16 2021 Mateusz Viste <mateusz@viste-family.net> 1.0.2
 - version 1.0.2 released
