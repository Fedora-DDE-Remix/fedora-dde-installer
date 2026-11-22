# Upstream and project identity

Fedora DDE Remix Installer is maintained for the Fedora-DDE-Remix project.
Its executable and package name is `fedora-dde-installer`.

The initial source is
<https://github.com/guanzi008/deepin-installer-reborn>, at commit
`3eaf0a53561673c5f07fafba292a66fae64ee75f` (the upstream `main` reviewed for this port).
That project derives from Deepin's installer. The original source copyright
notices and upstream history are retained; `debian/copyright` records the
upstream attribution. The C++ source notices specify GPL version 3 or later;
the GPL version 3 license text is in `COPYING`.

This Fedora port replaces the Debian package-management and immutable-system
deployment backend with a mutable Fedora installation backend. It uses one
Live session for installation and post-install account configuration. It does
not require a first-boot setup service to create the initial account.

The Fedora DDE Remix name, the generated blue/cyan installer icon, the Fedora
installation slides and project documentation identify this derivative.
The old deepin/UOS distribution artwork and promotional slides are not shipped.
Deepin component names such as DDE, DTK, DDM and Treeland continue to identify
the software components this installer uses.

The application icon in `configs/branding/fedora-dde-installer.png` was created
for this project with the built-in image generation tool. It is a blue/cyan
geometric D-shaped installation mark with a downward arrow, on transparency.
The same unmodified asset is included in the Qt resources. This is a community
Remix project, not an official Fedora installer.
