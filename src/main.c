#include <gba_interrupt.h>
#include <gba_input.h>
#include <gba_systemcalls.h>
#include <gba_video.h>

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 160
#define HUD_HEIGHT 20
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
#define HUD_TEXT_COLOR RGB5(20, 20, 20)
#define PLATFORM_COLOR RGB5(8, 15, 10)
#define PLATFORM_TOP_COLOR RGB5(16, 24, 19)
#define SHIP_BASE_COLOR RGB5(14, 10, 8)
#define SHIP_BASE_TOP_COLOR RGB5(22, 18, 14)
#define SHIP_SECTION_COLOR RGB5(26, 22, 12)
#define SHIP_PART_BASE_COLOR RGB5(30, 26, 10)
#define SHIP_PART_BODY_COLOR RGB5(12, 24, 20)
#define SHIP_PART_NOSE_COLOR RGB5(24, 18, 31)
#define FUEL_COLOR RGB5(31, 12, 8)
#define FUEL_DELIVERED_COLOR RGB5(20, 10, 6)
#define DELIVERY_ZONE_COLOR RGB5(4, 4, 4)
#define DELIVERY_ZONE_LINE_COLOR RGB5(8, 8, 8)
#define PLAYER_COLOR RGB5(31, 31, 31)
#define READY_TEXT_COLOR RGB5(12, 24, 12)
#define CLEAR_TEXT_COLOR RGB5(31, 20, 8)
#define PROJECTILE_COLOR RGB5(26, 24, 8)
#define PROJECTILE_TIP_COLOR RGB5(31, 31, 18)
#define DRAW_DELIVERY_ZONE_DEBUG 0
#define DRAW_LAUNCH_ZONE_DEBUG 0

typedef struct {
    int x;
    int y;
    int width;
    int height;
} Platform;

static const Platform platforms[] = {
    { 34, 62, 58, 4 },
    { 104, 88, 62, 4 },
    { 166, 42, 58, 4 }
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
    int type;
    int dropping;
    int delivered;
} ShipPart;

typedef struct {
    int x;
    int y;
    int targetX;
    int targetY;
    int state;
} Fuel;

#define PLATFORM_COUNT (sizeof(platforms) / sizeof(platforms[0]))

#define SHIP_BASE_WIDTH 20
#define SHIP_BASE_HEIGHT 6
#define SHIP_BASE_X 146
#define SHIP_BASE_Y (FLOOR_TOP_Y - SHIP_BASE_HEIGHT)
#define SHIP_BODY_WIDTH 12
#define SHIP_BODY_HEIGHT 22
#define SHIP_BODY_X (SHIP_BASE_X + 4)
#define SHIP_BODY_Y (SHIP_BASE_Y - SHIP_BODY_HEIGHT)
#define SHIP_NOSE_WIDTH 8
#define SHIP_NOSE_HEIGHT 10
#define SHIP_NOSE_X (SHIP_BASE_X + 6)
#define SHIP_NOSE_Y (SHIP_BODY_Y - SHIP_NOSE_HEIGHT)
#define SHIP_BOUNDS_X SHIP_BASE_X
#define SHIP_BOUNDS_Y SHIP_NOSE_Y
#define SHIP_BOUNDS_WIDTH SHIP_BASE_WIDTH
#define SHIP_BOUNDS_HEIGHT (FLOOR_TOP_Y - SHIP_NOSE_Y)
#define PLAYER_SPAWN_X 34

#define SHIP_PART_WIDTH 6
#define SHIP_PART_HEIGHT 6
#define SHIP_PART_COUNT 3
#define SHIP_PART_INDEX_BASE 0
#define SHIP_PART_INDEX_BODY 1
#define SHIP_PART_INDEX_NOSE 2
#define SHIP_PART_BASE 0
#define SHIP_PART_BODY 1
#define SHIP_PART_NOSE 2
#define SHIP_PART_CARRY_OFFSET_X 1
#define SHIP_PART_CARRY_OFFSET_Y -7
#define SHIP_PART_DROP_SPEED 1

#define PROJECTILE_WIDTH 18
#define PROJECTILE_HEIGHT 2
#define PROJECTILE_SPEED 7

#define SHIP_PART_SLOT0_X (SHIP_BASE_X + 7)
#define SHIP_PART_SLOT0_Y (SHIP_BODY_Y + 14)
#define SHIP_PART_SLOT1_X (SHIP_BASE_X + 7)
#define SHIP_PART_SLOT1_Y (SHIP_BODY_Y + 8)
#define SHIP_PART_SLOT2_X (SHIP_BASE_X + 7)
#define SHIP_PART_SLOT2_Y (SHIP_BODY_Y + 2)

#define SHIP_DELIVERY_COLUMN_WIDTH 36
#define SHIP_DELIVERY_COLUMN_X ((SHIP_BODY_X + (SHIP_BODY_WIDTH / 2)) - (SHIP_DELIVERY_COLUMN_WIDTH / 2))
#define SHIP_DELIVERY_COLUMN_Y PLAYFIELD_TOP
#define SHIP_DELIVERY_COLUMN_HEIGHT (FLOOR_TOP_Y - PLAYFIELD_TOP)
#define SHIP_LAUNCH_ZONE_X (SHIP_BASE_X - 4)
#define SHIP_LAUNCH_ZONE_Y (SHIP_NOSE_Y - 2)
#define SHIP_LAUNCH_ZONE_WIDTH (SHIP_BASE_WIDTH + 8)
#define SHIP_LAUNCH_ZONE_HEIGHT (SHIP_BOUNDS_HEIGHT + 2)

#define FUEL_WIDTH 6
#define FUEL_HEIGHT 6
#define FUEL_REQUIRED_COUNT 6
#define FUEL_CARRY_OFFSET_X 1
#define FUEL_CARRY_OFFSET_Y -7
#define FUEL_FALL_SPEED 1
#define FUEL_DROP_SPEED 1
#define FUEL_SPAWN_Y PLAYFIELD_TOP
#define FUEL_TARGET_X (SHIP_BASE_X + 1)
#define FUEL_TARGET_Y (SHIP_BASE_Y + 1)
#define FUEL_INACTIVE 0
#define FUEL_FALLING_FROM_SKY 1
#define FUEL_AVAILABLE 2
#define FUEL_CARRIED 3
#define FUEL_DROPPING_TO_SHIP 4
#define FUEL_DELIVERED 5

static const ShipPart shipPartDefaults[SHIP_PART_COUNT] = {
    { SHIP_PART_SLOT0_X, SHIP_PART_SLOT0_Y, SHIP_PART_SLOT0_X, SHIP_PART_SLOT0_Y, SHIP_PART_BASE, 0, 1 },
    { 60, 56, SHIP_PART_SLOT1_X, SHIP_PART_SLOT1_Y, SHIP_PART_BODY, 0, 0 },
    { 192, 36, SHIP_PART_SLOT2_X, SHIP_PART_SLOT2_Y, SHIP_PART_NOSE, 0, 0 }
};

static const Fuel fuelDefault = {
    0, FUEL_SPAWN_Y, FUEL_TARGET_X, FUEL_TARGET_Y, FUEL_INACTIVE
};

static const int fuelSpawnXs[FUEL_REQUIRED_COUNT] = { 36, 118, 192, 72, 170, 108 };

#define READY_TEXT_X 170
#define READY_TEXT_Y 26
#define CLEAR_TEXT_X 104
#define CLEAR_TEXT_Y 72

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

static void getFont3x5Rows(char c, unsigned char rows[5])
{
    int i;

    for (i = 0; i < 5; i++) {
        rows[i] = 0;
    }

    switch (c) {
        case '0': rows[0] = 0x7; rows[1] = 0x5; rows[2] = 0x5; rows[3] = 0x5; rows[4] = 0x7; break;
        case '1': rows[0] = 0x2; rows[1] = 0x6; rows[2] = 0x2; rows[3] = 0x2; rows[4] = 0x7; break;
        case '2': rows[0] = 0x7; rows[1] = 0x1; rows[2] = 0x7; rows[3] = 0x4; rows[4] = 0x7; break;
        case '3': rows[0] = 0x7; rows[1] = 0x1; rows[2] = 0x7; rows[3] = 0x1; rows[4] = 0x7; break;
        case '4': rows[0] = 0x5; rows[1] = 0x5; rows[2] = 0x7; rows[3] = 0x1; rows[4] = 0x1; break;
        case '5': rows[0] = 0x7; rows[1] = 0x4; rows[2] = 0x7; rows[3] = 0x1; rows[4] = 0x7; break;
        case '6': rows[0] = 0x7; rows[1] = 0x4; rows[2] = 0x7; rows[3] = 0x5; rows[4] = 0x7; break;
        case '7': rows[0] = 0x7; rows[1] = 0x1; rows[2] = 0x1; rows[3] = 0x1; rows[4] = 0x1; break;
        case '8': rows[0] = 0x7; rows[1] = 0x5; rows[2] = 0x7; rows[3] = 0x5; rows[4] = 0x7; break;
        case '9': rows[0] = 0x7; rows[1] = 0x5; rows[2] = 0x7; rows[3] = 0x1; rows[4] = 0x7; break;
        case 'A': rows[0] = 0x2; rows[1] = 0x5; rows[2] = 0x7; rows[3] = 0x5; rows[4] = 0x5; break;
        case 'C': rows[0] = 0x7; rows[1] = 0x4; rows[2] = 0x4; rows[3] = 0x4; rows[4] = 0x7; break;
        case 'D': rows[0] = 0x6; rows[1] = 0x5; rows[2] = 0x5; rows[3] = 0x5; rows[4] = 0x6; break;
        case 'E': rows[0] = 0x7; rows[1] = 0x4; rows[2] = 0x7; rows[3] = 0x4; rows[4] = 0x7; break;
        case 'H': rows[0] = 0x5; rows[1] = 0x5; rows[2] = 0x7; rows[3] = 0x5; rows[4] = 0x5; break;
        case 'I': rows[0] = 0x7; rows[1] = 0x2; rows[2] = 0x2; rows[3] = 0x2; rows[4] = 0x7; break;
        case 'L': rows[0] = 0x4; rows[1] = 0x4; rows[2] = 0x4; rows[3] = 0x4; rows[4] = 0x7; break;
        case 'O': rows[0] = 0x7; rows[1] = 0x5; rows[2] = 0x5; rows[3] = 0x5; rows[4] = 0x7; break;
        case 'R': rows[0] = 0x7; rows[1] = 0x5; rows[2] = 0x7; rows[3] = 0x5; rows[4] = 0x5; break;
        case 'S': rows[0] = 0x7; rows[1] = 0x4; rows[2] = 0x7; rows[3] = 0x1; rows[4] = 0x7; break;
        case 'V': rows[0] = 0x5; rows[1] = 0x5; rows[2] = 0x5; rows[3] = 0x5; rows[4] = 0x2; break;
        case 'Y': rows[0] = 0x5; rows[1] = 0x5; rows[2] = 0x2; rows[3] = 0x2; rows[4] = 0x2; break;
        default: break;
    }
}

static void drawChar3x5(int x, int y, char c, u16 color)
{
    unsigned char rows[5];
    int row;
    int col;

    getFont3x5Rows(c, rows);

    for (row = 0; row < 5; row++) {
        for (col = 0; col < 3; col++) {
            if (rows[row] & (1 << (2 - col))) {
                drawRect(x + col, y + row, 1, 1, color);
            }
        }
    }
}

static void drawText3x5(int x, int y, const char *text, u16 color)
{
    int cursorX = x;

    while (*text != '\0') {
        if (*text != ' ') {
            drawChar3x5(cursorX, y, *text, color);
        }
        cursorX += 4;
        text++;
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
    drawText3x5(8, 3, "SCORE", HUD_TEXT_COLOR);
    drawText3x5(97, 3, "LIVES", HUD_TEXT_COLOR);
    drawText3x5(196, 3, "HI", HUD_TEXT_COLOR);
    drawText3x5(8, 11, "000000", HUD_TEXT_COLOR);
    drawText3x5(105, 11, "4", HUD_TEXT_COLOR);
    drawText3x5(184, 11, "000000", HUD_TEXT_COLOR);
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
    drawRect(SHIP_BASE_X, SHIP_BASE_Y + 1, SHIP_BASE_WIDTH, SHIP_BASE_HEIGHT - 1, SHIP_BASE_COLOR);

    drawRect(SHIP_BODY_X, SHIP_BODY_Y, SHIP_BODY_WIDTH, 1, SHIP_BASE_TOP_COLOR);
    drawRect(SHIP_BODY_X, SHIP_BODY_Y + 1, SHIP_BODY_WIDTH, SHIP_BODY_HEIGHT - 1, SHIP_BASE_COLOR);

    drawRect(SHIP_NOSE_X, SHIP_NOSE_Y, SHIP_NOSE_WIDTH, 1, SHIP_BASE_TOP_COLOR);
    drawRect(SHIP_NOSE_X, SHIP_NOSE_Y + 1, SHIP_NOSE_WIDTH, SHIP_NOSE_HEIGHT - 1, SHIP_BASE_COLOR);
}

static void drawShipPart(const ShipPart *part)
{
    u16 color = SHIP_PART_BASE_COLOR;

    if (part->type == SHIP_PART_BODY) {
        color = SHIP_PART_BODY_COLOR;
    } else if (part->type == SHIP_PART_NOSE) {
        color = SHIP_PART_NOSE_COLOR;
    }

    drawRect(part->x, part->y, SHIP_PART_WIDTH, SHIP_PART_HEIGHT, color);
}

static void drawFuel(const Fuel *fuel)
{
    u16 color = fuel->state == FUEL_DELIVERED ? FUEL_DELIVERED_COLOR : FUEL_COLOR;
    drawRect(fuel->x, fuel->y, FUEL_WIDTH, FUEL_HEIGHT, color);
}

static void drawProjectile(int x, int y, int velX)
{
    drawRect(x, y, PROJECTILE_WIDTH, PROJECTILE_HEIGHT, PROJECTILE_COLOR);
    if (velX > 0) {
        drawRect(x + PROJECTILE_WIDTH - 2, y, 2, PROJECTILE_HEIGHT, PROJECTILE_TIP_COLOR);
    } else {
        drawRect(x, y, 2, PROJECTILE_HEIGHT, PROJECTILE_TIP_COLOR);
    }
}

static void drawDeliveryZone(void)
{
#if DRAW_DELIVERY_ZONE_DEBUG
    drawRect(
        SHIP_DELIVERY_COLUMN_X,
        SHIP_DELIVERY_COLUMN_Y,
        SHIP_DELIVERY_COLUMN_WIDTH,
        SHIP_DELIVERY_COLUMN_HEIGHT,
        DELIVERY_ZONE_COLOR
    );
    drawRect(
        SHIP_DELIVERY_COLUMN_X,
        SHIP_DELIVERY_COLUMN_Y + SHIP_DELIVERY_COLUMN_HEIGHT - 1,
        SHIP_DELIVERY_COLUMN_WIDTH,
        1,
        DELIVERY_ZONE_LINE_COLOR
    );
#endif
}

static void drawLaunchZone(void)
{
#if DRAW_LAUNCH_ZONE_DEBUG
    drawRect(
        SHIP_LAUNCH_ZONE_X,
        SHIP_LAUNCH_ZONE_Y,
        SHIP_LAUNCH_ZONE_WIDTH,
        SHIP_LAUNCH_ZONE_HEIGHT,
        DELIVERY_ZONE_COLOR
    );
    drawRect(
        SHIP_LAUNCH_ZONE_X,
        SHIP_LAUNCH_ZONE_Y + SHIP_LAUNCH_ZONE_HEIGHT - 1,
        SHIP_LAUNCH_ZONE_WIDTH,
        1,
        DELIVERY_ZONE_LINE_COLOR
    );
#endif
}

static void drawObjectiveIndicators(int shipReady, int stageClear)
{
    if (shipReady && !stageClear) {
        drawText3x5(READY_TEXT_X, READY_TEXT_Y, "READY", READY_TEXT_COLOR);
    }
    if (stageClear) {
        drawText3x5(CLEAR_TEXT_X, CLEAR_TEXT_Y, "CLEAR", CLEAR_TEXT_COLOR);
    }
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

static Rect getFuelRect(const Fuel *fuel)
{
    Rect rect;

    rect.x = fuel->x;
    rect.y = fuel->y;
    rect.width = FUEL_WIDTH;
    rect.height = FUEL_HEIGHT;
    return rect;
}

static Rect getProjectileRect(int x, int y)
{
    Rect rect;

    rect.x = x;
    rect.y = y;
    rect.width = PROJECTILE_WIDTH;
    rect.height = PROJECTILE_HEIGHT;
    return rect;
}

static int isFuelStaticVisible(const Fuel *fuel)
{
    return fuel->state == FUEL_AVAILABLE || fuel->state == FUEL_DELIVERED;
}

static int isFuelDynamicVisible(const Fuel *fuel)
{
    return (
        fuel->state == FUEL_FALLING_FROM_SKY ||
        fuel->state == FUEL_CARRIED ||
        fuel->state == FUEL_DROPPING_TO_SHIP
    );
}

static int rectIsValid(const Rect *rect)
{
    return rect->width > 0 && rect->height > 0;
}

static void redrawStaticInRect(
    const Rect *rect,
    const ShipPart *shipParts,
    const Fuel *fuel,
    int carriedPartIndex,
    int droppingPartIndex,
    int shipReady,
    int stageClear
)
{
    Rect hudRect;
    Rect floorRect;
    Rect platformRect;
    Rect shipBaseRect;
    Rect partRect;
    int i;

    if (!rectIsValid(rect)) {
        return;
    }

    hudRect.x = 0;
    hudRect.y = 0;
    hudRect.width = SCREEN_WIDTH;
    hudRect.height = HUD_HEIGHT;
    if (rectsOverlap(rect, &hudRect)) {
        drawHudPlaceholder();
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

    shipBaseRect.x = SHIP_BOUNDS_X;
    shipBaseRect.y = SHIP_BOUNDS_Y;
    shipBaseRect.width = SHIP_BOUNDS_WIDTH;
    shipBaseRect.height = SHIP_BOUNDS_HEIGHT;
    if (rectsOverlap(rect, &shipBaseRect)) {
        drawShipBase();
    }

    drawDeliveryZone();
    drawLaunchZone();
    drawObjectiveIndicators(shipReady, stageClear);

    for (i = 0; i < SHIP_PART_COUNT; i++) {
        if (i == carriedPartIndex || i == droppingPartIndex) {
            continue;
        }
        partRect = getShipPartRect(&shipParts[i]);
        if (rectsOverlap(rect, &partRect)) {
            drawShipPart(&shipParts[i]);
        }
    }

    if (isFuelStaticVisible(fuel)) {
        partRect = getFuelRect(fuel);
        if (rectsOverlap(rect, &partRect)) {
            drawFuel(fuel);
        }
    }
}

static void clearDynamicRect(
    const Rect *rect,
    const ShipPart *shipParts,
    const Fuel *fuel,
    int carriedPartIndex,
    int droppingPartIndex,
    int shipReady,
    int stageClear
)
{
    if (!rectIsValid(rect)) {
        return;
    }

    drawRect(rect->x, rect->y, rect->width, rect->height, SKY_COLOR);
    redrawStaticInRect(
        rect,
        shipParts,
        fuel,
        carriedPartIndex,
        droppingPartIndex,
        shipReady,
        stageClear
    );
}

static void drawStaticScene(
    const ShipPart *shipParts,
    const Fuel *fuel,
    int carriedPartIndex,
    int droppingPartIndex,
    int shipReady,
    int stageClear
)
{
    int i;

    fillScreen(SKY_COLOR);
    drawHudPlaceholder();
    drawFloor();
    drawPlatforms();
    drawShipBase();
    drawDeliveryZone();
    drawLaunchZone();
    drawObjectiveIndicators(shipReady, stageClear);
    for (i = 0; i < SHIP_PART_COUNT; i++) {
        if (i == carriedPartIndex || i == droppingPartIndex) {
            continue;
        }
        drawShipPart(&shipParts[i]);
    }
    if (isFuelStaticVisible(fuel)) {
        drawFuel(fuel);
    }
}

static int canDeliverShipPart(const ShipPart *shipParts, int partIndex)
{
    if (shipParts[partIndex].type == SHIP_PART_NOSE && !shipParts[SHIP_PART_INDEX_BODY].delivered) {
        return 0;
    }
    return 1;
}

static int canDeliverFuel(const ShipPart *shipParts)
{
    return shipParts[SHIP_PART_INDEX_BODY].delivered && shipParts[SHIP_PART_INDEX_NOSE].delivered;
}

static int isFuelObjectiveComplete(int deliveredFuelCount)
{
    return deliveredFuelCount >= FUEL_REQUIRED_COUNT;
}

static int isShipReady(const ShipPart *shipParts, int deliveredFuelCount)
{
    return (
        shipParts[SHIP_PART_INDEX_BODY].delivered &&
        shipParts[SHIP_PART_INDEX_NOSE].delivered &&
        deliveredFuelCount >= FUEL_REQUIRED_COUNT
    );
}

static int isShipAssembled(const ShipPart *shipParts)
{
    return shipParts[SHIP_PART_INDEX_BODY].delivered && shipParts[SHIP_PART_INDEX_NOSE].delivered;
}

static void spawnNextFuel(Fuel *fuel, int fuelSpawnIndex)
{
    if (fuelSpawnIndex < 0 || fuelSpawnIndex >= FUEL_REQUIRED_COUNT) {
        return;
    }

    fuel->x = fuelSpawnXs[fuelSpawnIndex];
    fuel->y = FUEL_SPAWN_Y;
    fuel->state = FUEL_FALLING_FROM_SKY;
}

static void updateFuelFallingFromSky(Fuel *fuel)
{
    int fuelLeft;
    int fuelRight;
    int previousBottomY;
    int currentBottomY;
    int bestPlatformTop;
    int landingFound;
    int previousFuelY;
    unsigned int i;

    if (fuel->state != FUEL_FALLING_FROM_SKY) {
        return;
    }

    previousFuelY = fuel->y;
    fuel->y += FUEL_FALL_SPEED;

    fuelLeft = fuel->x;
    fuelRight = fuelLeft + FUEL_WIDTH;
    previousBottomY = previousFuelY + FUEL_HEIGHT;
    currentBottomY = fuel->y + FUEL_HEIGHT;
    bestPlatformTop = SCREEN_HEIGHT + 1;
    landingFound = 0;

    for (i = 0; i < PLATFORM_COUNT; i++) {
        int platformLeft = platforms[i].x;
        int platformRight = platforms[i].x + platforms[i].width;
        int platformTop = platforms[i].y;

        if (fuelRight <= platformLeft || fuelLeft >= platformRight) {
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
        fuel->y = bestPlatformTop - FUEL_HEIGHT;
        fuel->state = FUEL_AVAILABLE;
        return;
    }

    if (fuel->y + FUEL_HEIGHT >= FLOOR_TOP_Y) {
        fuel->y = FLOOR_TOP_Y - FUEL_HEIGHT;
        fuel->state = FUEL_AVAILABLE;
    }
}

static void markPartChanged(int changedParts[], int *changedPartCount, int partIndex)
{
    int i;

    for (i = 0; i < *changedPartCount; i++) {
        if (changedParts[i] == partIndex) {
            return;
        }
    }

    changedParts[*changedPartCount] = partIndex;
    (*changedPartCount)++;
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

static void resolvePlayerHorizontalWrap(int *playerX)
{
    const int wrapLeft = -TO_FIX(PLAYER_WIDTH);
    const int wrapRight = TO_FIX(SCREEN_WIDTH);

    if (*playerX < wrapLeft) {
        *playerX = wrapRight;
    } else if (*playerX > wrapRight) {
        *playerX = wrapLeft;
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
    int droppingPartIndex;
    int deliveredFuelCount;
    int fuelSpawnIndex;
    int fuelObjectiveComplete;
    int shipReady;
    int stageClear;
    int playerFacing;
    int shipAssembled;
    int projectileActive;
    int oldProjectileActive;
    int projectileX;
    int projectileY;
    int projectileVelX;
    int i;
    int changedParts[SHIP_PART_COUNT];
    int changedPartCount;
    int fuelChanged;
    ShipPart shipParts[SHIP_PART_COUNT];
    ShipPart oldShipParts[SHIP_PART_COUNT];
    Fuel fuel;
    Fuel oldFuel;
    Rect playerRect;
    Rect oldPlayerRect;
    Rect shipDeliveryZoneRect;
    Rect shipLaunchZoneRect;
    Rect shipPartRect;
    Rect carriedPartRect;
    Rect fuelRect;
    Rect oldProjectileRect;
    Rect projectileRect;
    u16 keys;
    u16 keysPressed;

    const int minY = TO_FIX(PLAYFIELD_TOP);
    const int floorY = TO_FIX(FLOOR_TOP_Y - PLAYER_HEIGHT);

    irqInit();
    irqEnable(IRQ_VBLANK);

    REG_DISPCNT = MODE_3 | BG2_ON;

    playerX = TO_FIX(PLAYER_SPAWN_X);
    playerY = floorY;
    playerVelX = 0;
    playerVelY = 0;
    carriedPartIndex = -1;
    droppingPartIndex = -1;
    deliveredFuelCount = 0;
    fuelSpawnIndex = 0;
    fuelObjectiveComplete = 0;
    shipReady = 0;
    stageClear = 0;
    playerFacing = 1;
    projectileActive = 0;
    oldProjectileActive = 0;
    projectileX = 0;
    projectileY = 0;
    projectileVelX = 0;
    for (i = 0; i < SHIP_PART_COUNT; i++) {
        shipParts[i] = shipPartDefaults[i];
    }
    fuel = fuelDefault;
    shipDeliveryZoneRect.x = SHIP_DELIVERY_COLUMN_X;
    shipDeliveryZoneRect.y = SHIP_DELIVERY_COLUMN_Y;
    shipDeliveryZoneRect.width = SHIP_DELIVERY_COLUMN_WIDTH;
    shipDeliveryZoneRect.height = SHIP_DELIVERY_COLUMN_HEIGHT;
    shipLaunchZoneRect.x = SHIP_LAUNCH_ZONE_X;
    shipLaunchZoneRect.y = SHIP_LAUNCH_ZONE_Y;
    shipLaunchZoneRect.width = SHIP_LAUNCH_ZONE_WIDTH;
    shipLaunchZoneRect.height = SHIP_LAUNCH_ZONE_HEIGHT;

    drawStaticScene(shipParts, &fuel, carriedPartIndex, droppingPartIndex, shipReady, stageClear);
    drawRect(FROM_FIX(playerX), FROM_FIX(playerY), PLAYER_WIDTH, PLAYER_HEIGHT, PLAYER_COLOR);

    while (1) {
        VBlankIntrWait();
        scanKeys();
        keys = keysHeld();
        keysPressed = keysDown();

        oldPixelX = FROM_FIX(playerX);
        oldPixelY = FROM_FIX(playerY);
        oldPlayerRect = getPlayerRect(oldPixelX, oldPixelY);
        oldProjectileActive = projectileActive;
        if (oldProjectileActive) {
            oldProjectileRect = getProjectileRect(projectileX, projectileY);
        }
        for (i = 0; i < SHIP_PART_COUNT; i++) {
            oldShipParts[i] = shipParts[i];
        }
        oldFuel = fuel;
        changedPartCount = 0;
        fuelChanged = 0;

        if (keys & KEY_LEFT) {
            playerVelX -= PLAYER_MOVE_ACCEL;
            playerFacing = -1;
        } else if (keys & KEY_RIGHT) {
            playerVelX += PLAYER_MOVE_ACCEL;
            playerFacing = 1;
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
        resolvePlayerHorizontalWrap(&playerX);

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

        if ((keysPressed & KEY_B) && !projectileActive) {
            projectileActive = 1;
            projectileY = pixelY + (PLAYER_HEIGHT / 2) - (PROJECTILE_HEIGHT / 2);
            projectileVelX = playerFacing > 0 ? PROJECTILE_SPEED : -PROJECTILE_SPEED;
            if (playerFacing > 0) {
                projectileX = pixelX + PLAYER_WIDTH;
            } else {
                projectileX = pixelX - PROJECTILE_WIDTH;
            }
        }

        if (!stageClear && carriedPartIndex < 0 && droppingPartIndex < 0 && fuel.state != FUEL_CARRIED) {
            for (i = 0; i < SHIP_PART_COUNT; i++) {
                if (shipParts[i].delivered || shipParts[i].dropping) {
                    continue;
                }

                shipPartRect.x = shipParts[i].x;
                shipPartRect.y = shipParts[i].y;
                shipPartRect.width = SHIP_PART_WIDTH;
                shipPartRect.height = SHIP_PART_HEIGHT;

                if (rectsOverlap(&playerRect, &shipPartRect)) {
                    carriedPartIndex = i;
                    markPartChanged(changedParts, &changedPartCount, i);
                    break;
                }
            }
        }

        if (!stageClear) {
            shipAssembled = isShipAssembled(shipParts);
            if (
                fuel.state == FUEL_INACTIVE &&
                shipAssembled &&
                !fuelObjectiveComplete &&
                fuelSpawnIndex < FUEL_REQUIRED_COUNT
            ) {
                spawnNextFuel(&fuel, fuelSpawnIndex);
                fuelChanged = 1;
            }

            if (fuel.state == FUEL_FALLING_FROM_SKY) {
                updateFuelFallingFromSky(&fuel);
                fuelChanged = 1;
            }

            if (fuel.state == FUEL_AVAILABLE && carriedPartIndex < 0) {
                fuelRect = getFuelRect(&fuel);
                if (rectsOverlap(&playerRect, &fuelRect)) {
                    fuel.state = FUEL_CARRIED;
                    fuelChanged = 1;
                }
            }

            if (carriedPartIndex >= 0) {
                shipParts[carriedPartIndex].x = pixelX + SHIP_PART_CARRY_OFFSET_X;
                shipParts[carriedPartIndex].y = pixelY + SHIP_PART_CARRY_OFFSET_Y;
                if (shipParts[carriedPartIndex].y < PLAYFIELD_TOP) {
                    shipParts[carriedPartIndex].y = PLAYFIELD_TOP;
                }
                carriedPartRect = getShipPartRect(&shipParts[carriedPartIndex]);
                markPartChanged(changedParts, &changedPartCount, carriedPartIndex);

                if (rectsOverlap(&carriedPartRect, &shipDeliveryZoneRect)) {
                    if (canDeliverShipPart(shipParts, carriedPartIndex)) {
                        shipParts[carriedPartIndex].x = shipParts[carriedPartIndex].attachX;
                        if (shipParts[carriedPartIndex].y >= shipParts[carriedPartIndex].attachY) {
                            shipParts[carriedPartIndex].y = shipParts[carriedPartIndex].attachY - 1;
                        }
                        shipParts[carriedPartIndex].dropping = 1;
                        droppingPartIndex = carriedPartIndex;
                        markPartChanged(changedParts, &changedPartCount, carriedPartIndex);
                        carriedPartIndex = -1;
                    }
                }
            }

            if (droppingPartIndex >= 0) {
                shipParts[droppingPartIndex].y += SHIP_PART_DROP_SPEED;
                if (shipParts[droppingPartIndex].y >= shipParts[droppingPartIndex].attachY) {
                    shipParts[droppingPartIndex].y = shipParts[droppingPartIndex].attachY;
                    shipParts[droppingPartIndex].dropping = 0;
                    shipParts[droppingPartIndex].delivered = 1;
                    markPartChanged(changedParts, &changedPartCount, droppingPartIndex);
                    droppingPartIndex = -1;
                } else {
                    markPartChanged(changedParts, &changedPartCount, droppingPartIndex);
                }
            }

            if (fuel.state == FUEL_CARRIED) {
                fuel.x = pixelX + FUEL_CARRY_OFFSET_X;
                fuel.y = pixelY + FUEL_CARRY_OFFSET_Y;
                if (fuel.y < PLAYFIELD_TOP) {
                    fuel.y = PLAYFIELD_TOP;
                }
                fuelRect = getFuelRect(&fuel);
                fuelChanged = 1;

                if (rectsOverlap(&fuelRect, &shipDeliveryZoneRect) && canDeliverFuel(shipParts)) {
                    fuel.x = fuel.targetX;
                    if (fuel.y >= fuel.targetY) {
                        fuel.y = fuel.targetY - 1;
                    }
                    fuel.state = FUEL_DROPPING_TO_SHIP;
                    fuelChanged = 1;
                }
            }

            if (fuel.state == FUEL_DROPPING_TO_SHIP) {
                fuel.y += FUEL_DROP_SPEED;
                if (fuel.y >= fuel.targetY) {
                    fuel.y = fuel.targetY;
                    deliveredFuelCount++;
                    if (deliveredFuelCount > FUEL_REQUIRED_COUNT) {
                        deliveredFuelCount = FUEL_REQUIRED_COUNT;
                    }

                    if (isFuelObjectiveComplete(deliveredFuelCount)) {
                        fuel.state = FUEL_DELIVERED;
                        fuelObjectiveComplete = 1;
                    } else {
                        fuel.state = FUEL_INACTIVE;
                        fuelSpawnIndex++;
                    }
                }
                fuelChanged = 1;
            }
        }

        shipReady = isShipReady(shipParts, deliveredFuelCount);
        if (shipReady && !stageClear && rectsOverlap(&playerRect, &shipLaunchZoneRect)) {
            stageClear = 1;
            fuelChanged = 1;
        }

        if (projectileActive) {
            projectileX += projectileVelX;
            if (projectileX >= SCREEN_WIDTH || projectileX + PROJECTILE_WIDTH <= 0) {
                projectileActive = 0;
            }
        }

        clearDynamicRect(
            &oldPlayerRect,
            shipParts,
            &fuel,
            carriedPartIndex,
            droppingPartIndex,
            shipReady,
            stageClear
        );

        if (oldProjectileActive) {
            clearDynamicRect(
                &oldProjectileRect,
                shipParts,
                &fuel,
                carriedPartIndex,
                droppingPartIndex,
                shipReady,
                stageClear
            );
        }

        for (i = 0; i < changedPartCount; i++) {
            Rect oldPartRect = getShipPartRect(&oldShipParts[changedParts[i]]);
            clearDynamicRect(
                &oldPartRect,
                shipParts,
                &fuel,
                carriedPartIndex,
                droppingPartIndex,
                shipReady,
                stageClear
            );
        }

        if (fuelChanged) {
            Rect oldFuelRect = getFuelRect(&oldFuel);
            clearDynamicRect(
                &oldFuelRect,
                shipParts,
                &fuel,
                carriedPartIndex,
                droppingPartIndex,
                shipReady,
                stageClear
            );
        }

        if (carriedPartIndex >= 0) {
            drawShipPart(&shipParts[carriedPartIndex]);
        }
        if (droppingPartIndex >= 0) {
            drawShipPart(&shipParts[droppingPartIndex]);
        }
        if (isFuelDynamicVisible(&fuel)) {
            drawFuel(&fuel);
        }
        if (projectileActive) {
            projectileRect = getProjectileRect(projectileX, projectileY);
            drawProjectile(projectileRect.x, projectileRect.y, projectileVelX);
        }

        drawRect(playerRect.x, playerRect.y, PLAYER_WIDTH, PLAYER_HEIGHT, PLAYER_COLOR);
    }
}
