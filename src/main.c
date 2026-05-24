#include <gba_interrupt.h>
#include <gba_input.h>
#include <gba_systemcalls.h>
#include <gba_video.h>

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 160
#define HUD_HEIGHT 16
#define PLAYFIELD_TOP HUD_HEIGHT
#define PLAYFIELD_BOTTOM (SCREEN_HEIGHT - 1)

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

#define FLOOR_TOP_Y 146
#define FLOOR_HEIGHT ((PLAYFIELD_BOTTOM + 1) - FLOOR_TOP_Y)

#define SKY_COLOR RGB5(0, 0, 0)
#define FLOOR_COLOR RGB5(5, 5, 5)
#define FLOOR_TOP_COLOR RGB5(11, 11, 11)
#define HUD_COLOR RGB5(0, 0, 0)
#define HUD_LINE_COLOR RGB5(13, 13, 13)
#define PLATFORM_COLOR RGB5(8, 15, 10)
#define PLATFORM_TOP_COLOR RGB5(16, 24, 19)
#define SHIP_BASE_COLOR RGB5(14, 10, 8)
#define SHIP_BASE_TOP_COLOR RGB5(22, 18, 14)
#define SHIP_SECTION_COLOR RGB5(26, 22, 12)
#define SHIP_PART_COLOR RGB5(24, 22, 8)
#define SHIP_PART_DELIVERED_COLOR RGB5(30, 26, 10)
#define DELIVERY_ZONE_COLOR RGB5(4, 4, 4)
#define DELIVERY_ZONE_LINE_COLOR RGB5(8, 8, 8)
#define PLAYER_COLOR RGB5(31, 31, 31)
#define DRAW_DELIVERY_ZONE_DEBUG 0

typedef struct {
    int x;
    int y;
    int width;
    int height;
} Platform;

static const Platform platforms[] = {
    { 36, 112, 60, 6 },
    { 100, 88, 52, 6 },
    { 158, 58, 60, 6 }
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
#define SHIP_BASE_X 110
#define SHIP_BASE_Y (FLOOR_TOP_Y - SHIP_BASE_HEIGHT)

#define SHIP_PART_WIDTH 6
#define SHIP_PART_HEIGHT 6
#define SHIP_PART_COUNT 3
#define SHIP_PART_CARRY_OFFSET_X 1
#define SHIP_PART_CARRY_OFFSET_Y -7

#define SHIP_PART_SLOT0_X (SHIP_BASE_X + 7)
#define SHIP_PART_SLOT1_X (SHIP_BASE_X + 15)
#define SHIP_PART_SLOT2_X (SHIP_BASE_X + 23)
#define SHIP_PART_SLOT_Y (SHIP_BASE_Y - SHIP_PART_HEIGHT + 1)

#define SHIP_DELIVERY_ZONE_X (SHIP_BASE_X - 4)
#define SHIP_DELIVERY_ZONE_Y (SHIP_BASE_Y - 12)
#define SHIP_DELIVERY_ZONE_WIDTH (SHIP_BASE_WIDTH + 8)
#define SHIP_DELIVERY_ZONE_HEIGHT 12

static const ShipPart shipPartDefaults[SHIP_PART_COUNT] = {
    { SHIP_PART_SLOT0_X, SHIP_PART_SLOT_Y, SHIP_PART_SLOT0_X, SHIP_PART_SLOT_Y, 1 },
    { 62, 106, SHIP_PART_SLOT1_X, SHIP_PART_SLOT_Y, 0 },
    { 182, 52, SHIP_PART_SLOT2_X, SHIP_PART_SLOT_Y, 0 }
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

static void drawHudPlaceholder(void)
{
    drawRect(0, 0, SCREEN_WIDTH, HUD_HEIGHT, HUD_COLOR);
    drawRect(0, HUD_HEIGHT - 1, SCREEN_WIDTH, 1, HUD_LINE_COLOR);
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

static void drawDeliveryZone(void)
{
#if DRAW_DELIVERY_ZONE_DEBUG
    drawRect(
        SHIP_DELIVERY_ZONE_X,
        SHIP_DELIVERY_ZONE_Y,
        SHIP_DELIVERY_ZONE_WIDTH,
        SHIP_DELIVERY_ZONE_HEIGHT,
        DELIVERY_ZONE_COLOR
    );
    drawRect(
        SHIP_DELIVERY_ZONE_X,
        SHIP_DELIVERY_ZONE_Y + SHIP_DELIVERY_ZONE_HEIGHT - 1,
        SHIP_DELIVERY_ZONE_WIDTH,
        1,
        DELIVERY_ZONE_LINE_COLOR
    );
#endif
}

static Rect getPlayerRect(int playerPixelX, int playerPixelY)
{
    Rect rect;

    rect.x = playerPixelX;
    rect.y = playerPixelY;
    rect.width = PLAYER_WIDTH;
    rect.height = PLAYER_HEIGHT;
    return rect;
}

static Rect getShipPartRect(const ShipPart *part)
{
    Rect rect;

    rect.x = part->x;
    rect.y = part->y;
    rect.width = SHIP_PART_WIDTH;
    rect.height = SHIP_PART_HEIGHT;
    return rect;
}

static int rectIsValid(const Rect *rect)
{
    return rect->width > 0 && rect->height > 0;
}

static void redrawStaticInRect(const Rect *rect, const ShipPart *shipParts, int carriedPartIndex)
{
    Rect hudLineRect;
    Rect floorRect;
    Rect platformRect;
    Rect shipBaseRect;
    Rect partRect;
    int i;

    if (!rectIsValid(rect)) {
        return;
    }

    hudLineRect.x = 0;
    hudLineRect.y = HUD_HEIGHT - 1;
    hudLineRect.width = SCREEN_WIDTH;
    hudLineRect.height = 1;
    if (rectsOverlap(rect, &hudLineRect)) {
        drawRect(0, HUD_HEIGHT - 1, SCREEN_WIDTH, 1, HUD_LINE_COLOR);
    }

    floorRect.x = 0;
    floorRect.y = FLOOR_TOP_Y;
    floorRect.width = SCREEN_WIDTH;
    floorRect.height = FLOOR_HEIGHT;
    if (rectsOverlap(rect, &floorRect)) {
        drawFloor();
    }

    for (i = 0; i < PLATFORM_COUNT; i++) {
        platformRect.x = platforms[i].x;
        platformRect.y = platforms[i].y;
        platformRect.width = platforms[i].width;
        platformRect.height = platforms[i].height;
        if (rectsOverlap(rect, &platformRect)) {
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

    shipBaseRect.x = SHIP_BASE_X;
    shipBaseRect.y = SHIP_BASE_Y;
    shipBaseRect.width = SHIP_BASE_WIDTH;
    shipBaseRect.height = SHIP_BASE_HEIGHT;
    if (rectsOverlap(rect, &shipBaseRect)) {
        drawShipBase();
    }

    drawDeliveryZone();

    for (i = 0; i < SHIP_PART_COUNT; i++) {
        if (i == carriedPartIndex) {
            continue;
        }
        partRect = getShipPartRect(&shipParts[i]);
        if (rectsOverlap(rect, &partRect)) {
            drawShipPart(shipParts[i].x, shipParts[i].y, shipParts[i].delivered);
        }
    }
}

static void clearDynamicRect(const Rect *rect, const ShipPart *shipParts, int carriedPartIndex)
{
    if (!rectIsValid(rect)) {
        return;
    }

    drawRect(rect->x, rect->y, rect->width, rect->height, SKY_COLOR);
    redrawStaticInRect(rect, shipParts, carriedPartIndex);
}

static void drawStaticScene(const ShipPart *shipParts, int carriedPartIndex)
{
    int i;

    fillScreen(SKY_COLOR);
    drawHudPlaceholder();
    drawFloor();
    drawPlatforms();
    drawShipBase();
    drawDeliveryZone();
    for (i = 0; i < SHIP_PART_COUNT; i++) {
        if (i == carriedPartIndex) {
            continue;
        }
        drawShipPart(shipParts[i].x, shipParts[i].y, shipParts[i].delivered);
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
    int pixelX;
    int pixelY;
    int oldPixelX;
    int oldPixelY;
    int carriedPartIndex;
    int i;
    int changedParts[SHIP_PART_COUNT];
    int changedPartCount;
    ShipPart shipParts[SHIP_PART_COUNT];
    ShipPart oldShipParts[SHIP_PART_COUNT];
    Rect playerRect;
    Rect oldPlayerRect;
    Rect shipDeliveryZoneRect;
    Rect shipPartRect;
    Rect carriedPartNewRect;
    u16 keys;

    const int minX = 0;
    const int maxX = TO_FIX(SCREEN_WIDTH - PLAYER_WIDTH);
    const int minY = TO_FIX(PLAYFIELD_TOP);
    const int floorY = TO_FIX(FLOOR_TOP_Y - PLAYER_HEIGHT);

    irqInit();
    irqEnable(IRQ_VBLANK);

    REG_DISPCNT = MODE_3 | BG2_ON;

    playerX = TO_FIX((SCREEN_WIDTH / 2) - (PLAYER_WIDTH / 2));
    playerY = floorY;
    playerVelX = 0;
    playerVelY = 0;
    carriedPartIndex = -1;
    for (i = 0; i < SHIP_PART_COUNT; i++) {
        shipParts[i] = shipPartDefaults[i];
    }
    shipDeliveryZoneRect.x = SHIP_DELIVERY_ZONE_X;
    shipDeliveryZoneRect.y = SHIP_DELIVERY_ZONE_Y;
    shipDeliveryZoneRect.width = SHIP_DELIVERY_ZONE_WIDTH;
    shipDeliveryZoneRect.height = SHIP_DELIVERY_ZONE_HEIGHT;

    drawStaticScene(shipParts, carriedPartIndex);
    drawRect(FROM_FIX(playerX), FROM_FIX(playerY), PLAYER_WIDTH, PLAYER_HEIGHT, PLAYER_COLOR);

    while (1) {
        VBlankIntrWait();
        scanKeys();
        keys = keysHeld();

        oldPixelX = FROM_FIX(playerX);
        oldPixelY = FROM_FIX(playerY);
        oldPlayerRect = getPlayerRect(oldPixelX, oldPixelY);
        for (i = 0; i < SHIP_PART_COUNT; i++) {
            oldShipParts[i] = shipParts[i];
        }
        changedPartCount = 0;

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
        playerRect = getPlayerRect(pixelX, pixelY);

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
                    changedParts[changedPartCount++] = i;
                    break;
                }
            }
        }

        if (carriedPartIndex >= 0) {
            shipParts[carriedPartIndex].x = pixelX + SHIP_PART_CARRY_OFFSET_X;
            shipParts[carriedPartIndex].y = pixelY + SHIP_PART_CARRY_OFFSET_Y;
            if (shipParts[carriedPartIndex].y < PLAYFIELD_TOP) {
                shipParts[carriedPartIndex].y = PLAYFIELD_TOP;
            }
            changedParts[changedPartCount++] = carriedPartIndex;

            if (rectsOverlap(&playerRect, &shipDeliveryZoneRect)) {
                shipParts[carriedPartIndex].delivered = 1;
                shipParts[carriedPartIndex].x = shipParts[carriedPartIndex].attachX;
                shipParts[carriedPartIndex].y = shipParts[carriedPartIndex].attachY;
                changedParts[changedPartCount++] = carriedPartIndex;
                carriedPartIndex = -1;
            }
        }

        clearDynamicRect(&oldPlayerRect, shipParts, carriedPartIndex);

        for (i = 0; i < changedPartCount; i++) {
            Rect oldPartRect = getShipPartRect(&oldShipParts[changedParts[i]]);
            clearDynamicRect(&oldPartRect, shipParts, carriedPartIndex);
        }

        if (carriedPartIndex >= 0) {
            carriedPartNewRect = getShipPartRect(&shipParts[carriedPartIndex]);
            drawShipPart(
                carriedPartNewRect.x,
                carriedPartNewRect.y,
                shipParts[carriedPartIndex].delivered
            );
        }

        drawRect(playerRect.x, playerRect.y, PLAYER_WIDTH, PLAYER_HEIGHT, PLAYER_COLOR);
    }
}
