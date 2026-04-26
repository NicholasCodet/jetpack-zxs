#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_video.h>

static void fillScreen(u16 color)
{
    u16 *videoBuffer = (u16 *)VRAM;
    int i;

    for (i = 0; i < 240 * 160; i++) {
        videoBuffer[i] = color;
    }
}

int main(void)
{
    irqInit();
    irqEnable(IRQ_VBLANK);

    REG_DISPCNT = MODE_3 | BG2_ON;
    fillScreen(RGB5(2, 4, 10));

    while (1) {
        VBlankIntrWait();
    }
}
