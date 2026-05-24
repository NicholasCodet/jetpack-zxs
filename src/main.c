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
#define PLAYER_MOVE_ACCEL 32
#define PLAYER_FRICTION 20
#define PLAYER_MAX_MOVE_SPEED TO_FIX(1)
#define PLAYER_GRAVITY 20
#define PLAYER_THRUST 36
#define PLAYER_MAX_RISE_SPEED TO_FIX(2)
#define PLAYER_MAX_FALL_SPEED TO_FIX(2)

#define FLOOR_HEIGHT 8
#define FLOOR_TOP_Y (SCREEN_HEIGHT - FLOOR_HEIGHT)

#define SKY_COLOR RGB5(2, 4, 10)
#define FLOOR_COLOR RGB5(6, 6, 6)
#define FLOOR_TOP_COLOR RGB5(12, 12, 12)
#define PLATFORM_COLOR RGB5(8, 15, 10)
#define PLATFORM_TOP_COLOR RGB5(16, 24, 19)
#define SHIP_BASE_COLOR RGB5(14, 10, 8)
#define SHIP_BASE_TOP_COLOR RGB5(22, 18, 14)
#define SHIP_PART_COLOR RGB5(24, 22, 8)
#define SHIP_PART_DELIVERED_COLOR RGB5(30, 26, 10)
#define PLAYER_COLOR RGB5(31, 31, 31)

typedef struct {
    int x;
    int y;
    int width;
    int height;
} Platform;

static const Platform platforms[] = {
    { 24, 120, 64, 6 },
    { 104, 92, 72, 6 },
    { 174, 64, 52, 6 }
};

typedef struct {
    int x;
    int y;
    int width;
    int height;
} Rect;

typedef struct {
    int x;
    int y;
    int attachX;
    int attachY;
    int delivered;
} ShipPart;

#define PLATFORM_COUNT (sizeof(platforms) / sizeof(platforms[0]))

#define SHIP_BASE_WIDTH 36
#define SHIP_BASE_HEIGHT 10
#define SHIP_BASE_X 10
#define SHIP_BASE_Y (FLOOR_TOP_Y - SHIP_BASE_HEIGHT)

#define SHIP_PART_WIDTH 6
#define SHIP_PART_HEIGHT 6
#define SHIP_PART_COUNT 3
#define SHIP_PART_CARRY_OFFSET_X 1
#define SHIP_PART_CARRY_OFFSET_Y -7

static const ShipPart shipPartDefaults[SHIP_PART_COUNT] = {
    { 56, 114, SHIP_BASE_X + 7, SHIP_BASE_Y - SHIP_PART_HEIGHT + 1, 0 },
    { 132, 86, SHIP_BASE_X + 15, SHIP_BASE_Y - SHIP_PART_HEIGHT + 1, 0 },
    { 200, 58, SHIP_BASE_X + 23, SHIP_BASE_Y - SHIP_PART_HEIGHT + 1, 0 }
};

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

static void drawPlatforms(void)
{
    unsigned int i;

    for (i = 0; i < PLATFORM_COUNT; i++) {
        drawRect(platforms[i].x, platforms[i].y, platforms[i].width, 1, PLATFORM_TOP_COLOR);
        if (platforms[i].height > 1) {
            drawRect(
                platforms[i].x,
                platforms[i].y + 1,
                platforms[i].width,
                platforms[i].height - 1,
                PLATFORM_COLOR
            );
        }
    }
}

static void drawFloor(void)
{
    drawRect(0, FLOOR_TOP_Y, SCREEN_WIDTH, 1, FLOOR_TOP_COLOR);
    if (FLOOR_HEIGHT > 1) {
        drawRect(0, FLOOR_TOP_Y + 1, SCREEN_WIDTH, FLOOR_HEIGHT - 1, FLOOR_COLOR);
    }
}

static int rectsOverlap(const Rect *a, const Rect *b)
{
    if (a->x + a->width <= b->x) {
        return 0;
    }
    if (b->x + b->width <= a->x) {
        return 0;
    }
    if (a->y + a->height <= b->y) {
        return 0;
    }
    if (b->y + b->height <= a->y) {
        return 0;
    }
    return 1;
}

static void drawShipBase(void)
{
    drawRect(SHIP_BASE_X, SHIP_BASE_Y, SHIP_BASE_WIDTH, 1, SHIP_BASE_TOP_COLOR);
    if (SHIP_BASE_HEIGHT > 1) {
        drawRect(SHIP_BASE_X, SHIP_BASE_Y + 1, SHIP_BASE_WIDTH, SHIP_BASE_HEIGHT - 1, SHIP_BASE_COLOR);
    }
}

static void drawShipPart(int x, int y, int delivered)
{
    u16 color = delivered ? SHIP_PART_DELIVERED_COLOR : SHIP_PART_COLOR;
    drawRect(x, y, SHIP_PART_WIDTH, SHIP_PART_HEIGHT, color);
}

static void drawShipParts(const ShipPart *parts)
{
    int i;

    for (i = 0; i < SHIP_PART_COUNT; i++) {
        drawShipPart(parts[i].x, parts[i].y, parts[i].delivered);
    }
}

static void resolvePlatformLanding(int playerX, int previousPlayerY, int *playerY, int *playerVelY)
{
    int playerLeft;
    int playerRight;
    int previousBottomY;
    int currentBottomY;
    int bestPlatformTop;
    int landingFound;
    unsigned int i;

    if (*playerVelY < 0) {
        return;
    }

    playerLeft = FROM_FIX(playerX);
    playerRight = playerLeft + PLAYER_WIDTH;
    previousBottomY = previousPlayerY + TO_FIX(PLAYER_HEIGHT);
    currentBottomY = *playerY + TO_FIX(PLAYER_HEIGHT);
    bestPlatformTop = TO_FIX(SCREEN_HEIGHT + 1);
    landingFound = 0;

    for (i = 0; i < PLATFORM_COUNT; i++) {
        int platformLeft = platforms[i].x;
        int platformRight = platforms[i].x + platforms[i].width;
        int platformTop = TO_FIX(platforms[i].y);

        if (playerRight <= platformLeft || playerLeft >= platformRight) {
            continue;
        }
        if (previousBottomY > platformTop || currentBottomY < platformTop) {
            continue;
        }

        if (!landingFound || platformTop < bestPlatformTop) {
            bestPlatformTop = platformTop;
            landingFound = 1;
        }
    }

    if (landingFound) {
        *playerY = bestPlatformTop - TO_FIX(PLAYER_HEIGHT);
        *playerVelY = 0;
    }
}

int main(void)
{
    int playerX;
    int playerY;
    int playerVelX;
    int playerVelY;
    int prevPlayerY;
    int oldPixelX;
    int oldPixelY;
    int pixelX;
    int pixelY;
    int carriedPartIndex;
    int oldCarriedPartIndex;
    int needsRedraw;
    int i;
    int oldPartX[SHIP_PART_COUNT];
    int oldPartY[SHIP_PART_COUNT];
    int oldPartDelivered[SHIP_PART_COUNT];
    ShipPart shipParts[SHIP_PART_COUNT];
    Rect playerRect;
    Rect shipBaseRect;
    Rect shipPartRect;
    u16 keys;

    const int minX = 0;
    const int maxX = TO_FIX(SCREEN_WIDTH - PLAYER_WIDTH);
    const int minY = 0;
    const int floorY = TO_FIX(FLOOR_TOP_Y - PLAYER_HEIGHT);

    irqInit();
    irqEnable(IRQ_VBLANK);

    REG_DISPCNT = MODE_3 | BG2_ON;
    fillScreen(SKY_COLOR);
    drawFloor();
    drawPlatforms();

    playerX = TO_FIX((SCREEN_WIDTH / 2) - (PLAYER_WIDTH / 2));
    playerY = floorY;
    playerVelX = 0;
    playerVelY = 0;
    carriedPartIndex = -1;
    for (i = 0; i < SHIP_PART_COUNT; i++) {
        shipParts[i] = shipPartDefaults[i];
    }
    shipBaseRect.x = SHIP_BASE_X;
    shipBaseRect.y = SHIP_BASE_Y;
    shipBaseRect.width = SHIP_BASE_WIDTH;
    shipBaseRect.height = SHIP_BASE_HEIGHT;

    drawShipBase();
    drawShipParts(shipParts);
    drawRect(FROM_FIX(playerX), FROM_FIX(playerY), PLAYER_WIDTH, PLAYER_HEIGHT, PLAYER_COLOR);

    while (1) {
        VBlankIntrWait();
        scanKeys();
        keys = keysHeld();

        oldPixelX = FROM_FIX(playerX);
        oldPixelY = FROM_FIX(playerY);
        oldCarriedPartIndex = carriedPartIndex;
        for (i = 0; i < SHIP_PART_COUNT; i++) {
            oldPartX[i] = shipParts[i].x;
            oldPartY[i] = shipParts[i].y;
            oldPartDelivered[i] = shipParts[i].delivered;
        }

        if (keys & KEY_LEFT) {
            playerVelX -= PLAYER_MOVE_ACCEL;
        } else if (keys & KEY_RIGHT) {
            playerVelX += PLAYER_MOVE_ACCEL;
        } else if (playerVelX > 0) {
            playerVelX -= PLAYER_FRICTION;
            if (playerVelX < 0) {
                playerVelX = 0;
            }
        } else if (playerVelX < 0) {
            playerVelX += PLAYER_FRICTION;
            if (playerVelX > 0) {
                playerVelX = 0;
            }
        }

        if (playerVelX < -PLAYER_MAX_MOVE_SPEED) {
            playerVelX = -PLAYER_MAX_MOVE_SPEED;
        }
        if (playerVelX > PLAYER_MAX_MOVE_SPEED) {
            playerVelX = PLAYER_MAX_MOVE_SPEED;
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

        prevPlayerY = playerY;
        playerX += playerVelX;
        playerY += playerVelY;

        if (playerX < minX) {
            playerX = minX;
            playerVelX = 0;
        }
        if (playerX > maxX) {
            playerX = maxX;
            playerVelX = 0;
        }

        if (playerY < minY) {
            playerY = minY;
            if (playerVelY < 0) {
                playerVelY = 0;
            }
        }

        resolvePlatformLanding(playerX, prevPlayerY, &playerY, &playerVelY);

        if (playerY > floorY) {
            playerY = floorY;
            playerVelY = 0;
        }

        pixelX = FROM_FIX(playerX);
        pixelY = FROM_FIX(playerY);
        playerRect.x = pixelX;
        playerRect.y = pixelY;
        playerRect.width = PLAYER_WIDTH;
        playerRect.height = PLAYER_HEIGHT;

        if (carriedPartIndex < 0) {
            for (i = 0; i < SHIP_PART_COUNT; i++) {
                if (shipParts[i].delivered) {
                    continue;
                }

                shipPartRect.x = shipParts[i].x;
                shipPartRect.y = shipParts[i].y;
                shipPartRect.width = SHIP_PART_WIDTH;
                shipPartRect.height = SHIP_PART_HEIGHT;

                if (rectsOverlap(&playerRect, &shipPartRect)) {
                    carriedPartIndex = i;
                    break;
                }
            }
        }

        if (carriedPartIndex >= 0) {
            shipParts[carriedPartIndex].x = pixelX + SHIP_PART_CARRY_OFFSET_X;
            shipParts[carriedPartIndex].y = pixelY + SHIP_PART_CARRY_OFFSET_Y;

            if (rectsOverlap(&playerRect, &shipBaseRect)) {
                shipParts[carriedPartIndex].delivered = 1;
                shipParts[carriedPartIndex].x = shipParts[carriedPartIndex].attachX;
                shipParts[carriedPartIndex].y = shipParts[carriedPartIndex].attachY;
                carriedPartIndex = -1;
            }
        }

        needsRedraw = 0;
        if (pixelX != oldPixelX || pixelY != oldPixelY) {
            needsRedraw = 1;
        }
        if (carriedPartIndex != oldCarriedPartIndex) {
            needsRedraw = 1;
        }
        for (i = 0; i < SHIP_PART_COUNT; i++) {
            if (shipParts[i].x != oldPartX[i] || shipParts[i].y != oldPartY[i] || shipParts[i].delivered != oldPartDelivered[i]) {
                needsRedraw = 1;
                break;
            }
        }

        if (needsRedraw) {
            drawRect(oldPixelX, oldPixelY, PLAYER_WIDTH, PLAYER_HEIGHT, SKY_COLOR);
            for (i = 0; i < SHIP_PART_COUNT; i++) {
                drawRect(oldPartX[i], oldPartY[i], SHIP_PART_WIDTH, SHIP_PART_HEIGHT, SKY_COLOR);
            }
            drawFloor();
            drawPlatforms();
            drawShipBase();
            drawShipParts(shipParts);
            drawRect(pixelX, pixelY, PLAYER_WIDTH, PLAYER_HEIGHT, PLAYER_COLOR);
        }
    }
}
