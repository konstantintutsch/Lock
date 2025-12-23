[private]
default:
    just --list --justfile {{ justfile() }}

format:
    indent src/*.c src/*.h -linux -nut -i4 -l1024

translate:
    meson compile -C _meson com.konstantintutsch.Lock-pot
    meson compile -C _meson com.konstantintutsch.Lock-update-po

local:
    flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
    flatpak run org.flatpak.Builder --user --sandbox \
        --force-clean --ccache --install-deps-from=flathub \
        --repo=_repo _flatpak \
        com.konstantintutsch.Lock.Devel.yaml
    flatpak remote-add --if-not-exists --user --no-gpg-verify \
        com-konstantintutsch-Lock-Devel \
        file://$(pwd)/_repo
    flatpak install --user --reinstall --assumeyes \
        --include-sdk --include-debug \
        com.konstantintutsch.Lock.Devel
    just run

run:
    GTK_DEBUG=interactive flatpak run \
        com.konstantintutsch.Lock.Devel

debug:
    flatpak-coredumpctl \
        -m $(coredumpctl list -1 --no-pager --no-legend | grep -oE 'CEST ([0-9]+)' | awk '{print $2}') \
        com.konstantintutsch.Lock.Devel

bundle:
    flatpak build-bundle _repo _com.konstantintutsch.Lock.Devel.flatpak \
        --runtime-repo=https://flathub.org/repo/flathub.flatpakrepo \
        com.konstantintutsch.Lock.Devel

dist:
    rm --verbose --interactive=never --recursive _*
    rm --verbose --interactive=never --recursive .flatpak-builder
    rm --verbose --interactive=never --recursive .cache
    rm --verbose --interactive=never --recursive compile_commands.json

setup:
    sudo dnf install --assumeyes \
        indent \
        meson \
        bear \
        libadwaita-devel \
        gpgme-devel
    flatpak install --user --assumeyes \
        org.gnome.Platform//49 \
        org.gnome.Sdk//49 \
        org.gnome.Sdk.Debug//49 \
        org.flatpak.Builder

    meson setup _meson
    bear -- meson compile -C _meson
