FROM wiiuenv/devkitppc:20221228

COPY --from=wiiuenv/wiiupluginsystem:20230126 /artifacts $DEVKITPRO
COPY --from=wiiuenv/libnotifications:20230126 /artifacts $DEVKITPRO
COPY --from=wiiuenv/librpxloader:20220903 /artifacts $DEVKITPRO
COPY --from=wiiuenv/libcurlwrapper:20230121 /artifacts $DEVKITPRO

WORKDIR project