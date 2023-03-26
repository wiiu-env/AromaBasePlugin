FROM ghcr.io/wiiu-env/devkitppc:20230326

COPY --from=ghcr.io/wiiu-env/wiiupluginsystem:20230215 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libnotifications:20230126 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/librpxloader:20220903 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libcurlwrapper:20230121 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libsdutils:20220903 /artifacts $DEVKITPRO

WORKDIR project