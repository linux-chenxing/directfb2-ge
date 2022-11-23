# DirectFB2 driver for MStar/SigmaStar GE

This is the DFB2 part of hardware accelerated 2D graphics
for MStar/SigmaStar machines. This is very much Work In Progress!

You also need the libge userspace part of the driver:
- https://github.com/linux-chenxing/libge.

``` mermaid
graph TD
    APP["DirectFB2 app(SDL2 etc)"] --> DFB2(DirectFB2)
    DFB2 --> DRMKMSSS(DRM/KMS system)
    DFB2 -- "line, rect, blit op" --> DFB2GE(DirectFB2 GE driver)
    DRMKMSSS --> DRMKMS(DRM/KMS)
    DRMKMS --> DRMKMSBUF(DRM/KMS buffers)
    DFB2GE --> LIBGE(linux-chenxing libge)
    LIBGE -- ioctl --> GE(MStar/SigmaStar GE)
    GE -- "DMA into buffer" --> DRMKMSBUF
    DRMKMSBUF -- "prime fd" --> GE
    DRMKMSBUF -- "framebuffer" --> GOP(MStar/SigmaStar GOP)
    GOP -- "scan out" --> DISP("Display (LCD, HDMI etc)")
```
