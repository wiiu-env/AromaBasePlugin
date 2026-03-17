FROM ghcr.io/wiiu-env/devkitppc:20260225

COPY --from=ghcr.io/wiiu-env/wiiupluginsystem:20260208 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libnotifications:20260131 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/librpxloader:20260207 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libcurlwrapper:20260207 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libsdutils:20260207 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libmocha:20260126 /artifacts $DEVKITPRO

WORKDIR project
