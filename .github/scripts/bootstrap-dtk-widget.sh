#!/bin/bash
# Build the missing SDK RPM in the disposable Fedora CI container. This does
# not publish packages or change the desktop project's COPR repository.
set -euo pipefail

if (( EUID != 0 )); then
    printf '%s\n' 'Run this helper only inside the disposable Fedora build container.' >&2
    exit 1
fi

readonly recipe_commit=7b32215a3ada9664e52382247be9a5823bcc8617
readonly recipe_sha256=a6fbab31190916d15293dbe728590a876afc88a320706edac1ff41ebc2f0c0c4
readonly archive_sha256=299c64f46c20f078a055f4b69242a9750c84be4558e1dce3af2aa53a66fcaead
readonly sdk_version=6.7.54
readonly sdk_top="$PWD/sdk-rpmbuild"
readonly recipe="$sdk_top/SPECS/dtk6widget.spec"
readonly archive="$sdk_top/SOURCES/dtkwidget-$sdk_version.tar.gz"

mkdir -p "$sdk_top"/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}
curl --fail --location --retry 3 --output "$recipe" \
    "https://raw.githubusercontent.com/Fedora-DDE-Remix/Deepin-Desktop-Environment/$recipe_commit/dtkwidget/dtk6widget.spec"
curl --fail --location --retry 3 --output "$archive" \
    "https://github.com/linuxdeepin/dtkwidget/archive/refs/tags/$sdk_version.tar.gz"
printf '%s  %s\n' "$recipe_sha256" "$recipe" "$archive_sha256" "$archive" |
    sha256sum --check
printf '%s\n' \
    "recipe_commit=$recipe_commit" \
    "recipe_sha256=$recipe_sha256" \
    "archive_version=$sdk_version" \
    "archive_sha256=$archive_sha256" > "$sdk_top/SOURCES.lock"

# Gui 6.7.54-2 already has a successful Fedora 45 x86_64 COPR build
# (11374228), with cmake(Dtk6Gui) in its actual RPM Provides. Refresh metadata
# before resolving it. Log's development CMake package is also required when
# the Core package is loaded; the current Core-devel RPM does not require it.
if ! dnf -y --refresh install 'cmake(Dtk6Gui)' 'cmake(Dtk6Log)'; then
    printf '%s\n' '::error::The existing Dtk6Gui/Dtk6Log development RPMs are not available from the configured repositories. The Widget SDK cannot be bootstrapped until those packages are published.' >&2
    exit 1
fi
dnf -y builddep "$recipe"

# Keep the pinned recipe unchanged, including its documentation and plugin
# build options, because its RPM file lists include those outputs.
rpmbuild --define "_topdir $sdk_top" -ba "$recipe" 2>&1 |
    tee "$sdk_top/build.log"
shopt -s globstar nullglob
sdk_rpms=(
    "$sdk_top"/RPMS/**/dtk6widget-"$sdk_version"-*.rpm
    "$sdk_top"/RPMS/**/dtk6widget-devel-"$sdk_version"-*.rpm
)
if (( ${#sdk_rpms[@]} != 2 )); then
    printf '%s\n' 'Expected one Widget runtime RPM and one development RPM.' >&2
    exit 1
fi
dnf -y install "${sdk_rpms[@]}"
rpm -q --whatprovides 'cmake(Dtk6Widget)'
