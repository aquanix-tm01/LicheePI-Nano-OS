# Fast cross-compile of myapp only, on top of the base image (no kernel/U-Boot rebuild).
ARG BASE_IMAGE=licheepi-nano-base
ARG BASE_VERSION=latest

FROM $BASE_IMAGE:$BASE_VERSION AS app
RUN rm -rf /root/licheepi-nano /root/myapp
COPY br2-external/ /root/licheepi-nano/
COPY myapp/ /root/myapp/

WORKDIR /root/buildroot
RUN BR2_EXTERNAL=/root/licheepi-nano make licheepi_nano_defconfig \
 && echo 'BR2_PACKAGE_MYAPP=y' >> .config \
 && make olddefconfig
RUN make myapp

FROM scratch AS appout
COPY --from=app /root/buildroot/output/target/usr/bin/myapp /myapp
