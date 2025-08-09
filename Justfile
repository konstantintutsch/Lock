[private]
default:
    just --list --justfile {{ justfile() }}

format:
    indent src/*.c src/*.h -linux -nut -i4

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

setup:
    sudo dnf install -y indent
    sudo dnf install -y meson
    sudo dnf install -y libadwaita-devel
    sudo dnf install -y gpgme-devel
    flatpak install --user --assumeyes org.gnome.Platform//48
    flatpak install --user --assumeyes org.gnome.Sdk//48
    flatpak install --user --assumeyes org.gnome.Sdk.Debug//48
    flatpak install --user --assumeyes org.flatpak.Builder
