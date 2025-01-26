FROM ghcr.io/wiiu-env/devkitppc:20241128

COPY --from=ghcr.io/wiiu-env/wiiupluginsystem:0.8.2-dev-20250125-f8faa9f /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libnotifications:20240426 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/librpxloader:20240425 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libcurlwrapper:20240505 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libsdutils:20230621 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libmocha:20231127 /artifacts $DEVKITPRO

WORKDIR project
