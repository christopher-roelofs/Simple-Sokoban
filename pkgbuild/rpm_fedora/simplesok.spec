Name:		simplesok
Version:	1.0.2
Release:	1%{?dist}
Summary:	Simple Sokoban is a colorful Sokoban game
License:	MIT and CC-BY
URL:		http://simplesok.sourceforge.net/
Source0:	http://downloads.sourceforge.net/%{name}/%{name}-%{version}.tar.xz
Source1:	simplesok-1.0.2-ibm-cga.bmp.gz
Patch1:		simplesok-1.0.2-Avoid-dereferencing-an-uninitialized-value.patch
Patch2:		simplesok-1.0.2-Reorganize-sok_loadfile-to-avoid-double-free-on-erro.patch
Patch3:		simplesok-1.0.2-Rewrite-network-interface-to-use-libcurl.patch
Patch4:		simplesok-1.0.2-Skin-avoid-double-free-of-font-SDL-texture.patch

BuildRequires:	gcc
BuildRequires:	SDL2-devel >= 2.0.1
BuildRequires:	libcurl-devel
BuildRequires:	zlib-devel
BuildRequires:	desktop-file-utils
Requires:	hicolor-icon-theme

%description
Simple Sokoban is a colorful Sokoban game aimed for playability and portability
across systems. It is written in ANSI C89, using SDL2 for user interactions.


#-------------------------------------------------------------------------------
%prep
#-------------------------------------------------------------------------------

%autosetup -p1

cp '%{SOURCE1}' skins/ibm-cga.bmp.gz

#	Convert man page encoding.

iconv -f ISO-8859-1 -t UTF-8 -o t simplesok.6 && mv t simplesok.6

#	Convert to NL line ending.
for TXT in *.txt
do	sed -i 's/\r$//' ${TXT}
done


#-------------------------------------------------------------------------------
%build
#-------------------------------------------------------------------------------

make CFLAGS='%{optflags} -Wall -Wextra -std=gnu89 -pedantic -Wno-long-long -Wformat-security'


#-------------------------------------------------------------------------------
%install
#-------------------------------------------------------------------------------

#	Install application.
install -D -m 755 simplesok '%{buildroot}%{_bindir}/simplesok'
install -D -m 644 --target-directory '%buildroot/usr/share/simplesok/skins/' \
		skins/*.bmp.gz

#	Install desktop menu entry.
mkdir -p '%{buildroot}%{_datadir}/applications'
cat > '%{name}.desktop' << EOF
[Desktop Entry]
Name=Simplesok
Comment=Slide boxes to solve the puzzles
Comment[fr]=Poussez les boîtes pour résoudre les casse-têtes
Exec=%{name}
Icon=%{name}
Terminal=false
Type=Application
Categories=Game;LogicGame;
EOF

desktop-file-install							\
	--dir "%{buildroot}/%{_datadir}/applications"			\
	"%{name}.desktop"

install -D -m 644 simplesok.png						\
		'%{buildroot}%{_datadir}/icons/hicolor/48x48/apps/simplesok.png'

#	Install man page.
install -D -m 644 simplesok.6 '%buildroot/%{_mandir}/man6/simplesok.6'


#-------------------------------------------------------------------------------
%files
#-------------------------------------------------------------------------------

%doc simplesok.txt history.txt xsb_format.txt
%{_bindir}/simplesok
%{_datadir}/simplesok/skins/*.bmp.gz
%{_datadir}/applications/*
%{_datadir}/icons/hicolor/48x48/apps/simplesok.png
%{_mandir}/man6/simplesok.6*


#-------------------------------------------------------------------------------
%changelog
#-------------------------------------------------------------------------------

* Sun Feb 12 2023 Patrick Monnerat <patrick@monnerat.net> 1.0.2-1
- Initial Fedora spec file adapted from project's spec file.

* Thu Dec 16 2021 Mateusz Viste <mateusz@viste-family.net> 1.0.2
 - version 1.0.2 released
