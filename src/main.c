#include <gba_interrupt.h>
#include <gba_input.h>
#include <gba_systemcalls.h>
#include <gba_video.h>

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 160

#define FIX_SHIFT 8
#define TO_FIX(x) ((x) << FIX_SHIFT)
#define FROM_FIX(x) ((x) >> FIX_SHIFT)

// Tunable movement constants (8.8 fixed-point for velocities/positions).
#define PLAYER_WIDTH 8
#define PLAYER_HEIGHT 8
#define PLAYER_MOVE_SPEED TO_FIX(1)
#define PLAYER_GRAVITY 20
#define PLAYER_THRUST 36
#define PLAYER_MAX_RISE_SPEED TO_FIX(2)
#define PLAYER_MAX_FALL_SPEED TO_FIX(2)

#define FLOOR_HEIGHT 8
#define FLOOR_TOP_Y (SCREEN_HEIGHT - FLOOR_HEIGHT)

#define SKY_COLOR RGB5(2, 4, 10)
#define FLOOR_COLOR RGB5(6, 6, 6)
#define PLAYER_COLOR RGB5(31, 31, 31)

static void fillScreen(u16 color)
{
    u16 *videoBuffer = (u16 *)VRAM;
    int i;

    for (i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        videoBuffer[i] = color;
    }
}

static void drawRect(int x, int y, int width, int height, u16 color)
{
    int startX;
    int startY;
    int endX;
    int endY;
    int yy;
    int xx;

    startX = x < 0 ? 0 : x;
    startY = y < 0 ? 0 : y;
    endX = x + width;
    endY = y + height;

    if (endX > SCREEN_WIDTH) {
        endX = SCREEN_WIDTH;
    }
    if (endY > SCREEN_HEIGHT) {
        endY = SCREEN_HEIGHT;
    }
    if (startX >= endX || startY >= endY) {
        return;
    }

    for (yy = startY; yy < endY; yy++) {
        u16 *row = (u16 *)VRAM + (yy * SCREEN_WIDTH) + startX;
        for (xx = startX; xx < endX; xx++) {
            row[xx - startX] = color;
        }
    }
}

int main(void)
{
    int playerX;
    int playerY;
    int playerVelY;
    int oldPixelX;
    int oldPixelY;
    int pixelX;
    int pixelY;
    u16 keys;

    const int minX = 0;
    const int maxX = TO_FIX(SCREEN_WIDTH - PLAYER_WIDTH);
    const int minY = 0;
    const int floorY = TO_FIX(FLOOR_TOP_Y - PLAYER_HEIGHT);

    irqInit();
    irqEnable(IRQ_VBLANK);

    REG_DISPCNT = MODE_3 | BG2_ON;
    fillScreen(SKY_COLOR);
    drawRect(0, FLOOR_TOP_Y, SCREEN_WIDTH, FLOOR_HEIGHT, FLOOR_COLOR);

    playerX = TO_FIX((SCREEN_WIDTH / 2) - (PLAYER_WIDTH / 2));
    playerY = floorY;
    playerVelY = 0;

    drawRect(FROM_FIX(playerX), FROM_FIX(playerY), PLAYER_WIDTH, PLAYER_HEIGHT, PLAYER_COLOR);

    while (1) {
        VBlankIntrWait();
        scanKeys();
        keys = keysHeld();

        oldPixelX = FROM_FIX(playerX);
        oldPixelY = FROM_FIX(playerY);

        if (keys & KEY_LEFT) {
            playerX -= PLAYER_MOVE_SPEED;
        } else if (keys & KEY_RIGHT) {
            playerX += PLAYER_MOVE_SPEED;
        }

        if (keys & KEY_A) {
            playerVelY -= PLAYER_THRUST;
        }
        playerVelY += PLAYER_GRAVITY;

        if (playerVelY < -PLAYER_MAX_RISE_SPEED) {
            playerVelY = -PLAYER_MAX_RISE_SPEED;
        }
        if (playerVelY > PLAYER_MAX_FALL_SPEED) {
            playerVelY = PLAYER_MAX_FALL_SPEED;
        }

        playerY += playerVelY;

        if (playerX < minX) {
            playerX = minX;
        }
        if (playerX > maxX) {
            playerX = maxX;
        }

        if (playerY < minY) {
            playerY = minY;
            if (playerVelY < 0) {
                playerVelY = 0;
            }
        }

        if (playerY > floorY) {
            playerY = floorY;
            playerVelY = 0;
        }

        pixelX = FROM_FIX(playerX);
        pixelY = FROM_FIX(playerY);

        if (pixelX != oldPixelX || pixelY != oldPixelY) {
            drawRect(oldPixelX, oldPixelY, PLAYER_WIDTH, PLAYER_HEIGHT, SKY_COLOR);
            drawRect(pixelX, pixelY, PLAYER_WIDTH, PLAYER_HEIGHT, PLAYER_COLOR);
        }
    }
}
