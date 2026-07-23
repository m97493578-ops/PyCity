// PyCity - first build
// A top-down tile-grid city sim: place roads and buildings, trucks path
// between buildings automatically along the road network.
//
// Controls:
//   1 = Road tool     2 = House tool     3 = Factory tool     4 = Bulldoze
//   Left click        = place/remove on hovered tile
//   Right click / drag pan is not needed, map fits on screen
//   Space             = pause/unpause truck simulation
//   Esc               = quit
//
// Build: gcc main.c -o pycity -Iraylib/src -Lraylib/src -lraylib -lm -lpthread -ldl -lrt -lX11
// Run:   ./pycity

#include "raylib.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define COLS 30
#define ROWS 20
#define TILE 32
#define SCREEN_W (COLS*TILE)
#define SCREEN_H (ROWS*TILE + 60) // +60 for the toolbar at top
#define TOP_BAR 60
#define MAX_TRUCKS 64
#define MAX_BUILDINGS 64
#define MAX_PATH (COLS*ROWS)

typedef enum { TILE_EMPTY = 0, TILE_ROAD, TILE_HOUSE, TILE_FACTORY } TileType;
typedef enum { TOOL_ROAD = 0, TOOL_HOUSE, TOOL_FACTORY, TOOL_BULLDOZE } Tool;

static TileType grid[ROWS][COLS];

typedef struct {
    int r, c;
    TileType type;
} Building;

static Building buildings[MAX_BUILDINGS];
static int buildingCount = 0;

typedef struct {
    int pathR[MAX_PATH];
    int pathC[MAX_PATH];
    int pathLen;
    int idx;      // current segment index
    float t;      // 0..1 progress along current segment
    float speed;  // per-frame progress
    Color color;
    bool active;
} Truck;

static Truck trucks[MAX_TRUCKS];
static int deliveries = 0;

// ---- helpers ----

static Vector2 CellCenter(int r, int c) {
    Vector2 v = { c*TILE + TILE/2.0f, TOP_BAR + r*TILE + TILE/2.0f };
    return v;
}

static bool InBounds(int r, int c) {
    return r >= 0 && r < ROWS && c >= 0 && c < COLS;
}

static int RoadNeighbors(int r, int c, int outR[4], int outC[4]) {
    int n = 0;
    int dr[4] = {0,0,1,-1};
    int dc[4] = {1,-1,0,0};
    for (int i = 0; i < 4; i++) {
        int nr = r+dr[i], nc = c+dc[i];
        if (InBounds(nr,nc) && grid[nr][nc] == TILE_ROAD) {
            outR[n] = nr; outC[n] = nc; n++;
        }
    }
    return n;
}

// Find nearest road tile to a building (simple expanding search)
static bool NearestRoad(int br, int bc, int *outR, int *outC) {
    for (int radius = 0; radius < COLS+ROWS; radius++) {
        for (int dr = -radius; dr <= radius; dr++) {
            for (int dc = -radius; dc <= radius; dc++) {
                if (abs(dr)+abs(dc) != radius) continue;
                int nr = br+dr, nc = bc+dc;
                if (InBounds(nr,nc) && grid[nr][nc] == TILE_ROAD) {
                    *outR = nr; *outC = nc;
                    return true;
                }
            }
        }
    }
    return false;
}

// BFS pathfinding across road tiles, writes path into truck, returns success
static bool BFSPath(int sr, int sc, int er, int ec, Truck *tr) {
    static int prevR[ROWS][COLS], prevC[ROWS][COLS];
    static bool visited[ROWS][COLS];
    memset(visited, 0, sizeof(visited));

    int queueR[MAX_PATH], queueC[MAX_PATH];
    int qHead = 0, qTail = 0;
    queueR[qTail] = sr; queueC[qTail] = sc; qTail++;
    visited[sr][sc] = true;
    prevR[sr][sc] = -1; prevC[sr][sc] = -1;

    bool found = false;
    while (qHead < qTail) {
        int r = queueR[qHead], c = queueC[qHead]; qHead++;
        if (r == er && c == ec) { found = true; break; }
        int nr[4], nc[4];
        int n = RoadNeighbors(r, c, nr, nc);
        for (int i = 0; i < n; i++) {
            if (!visited[nr[i]][nc[i]]) {
                visited[nr[i]][nc[i]] = true;
                prevR[nr[i]][nc[i]] = r;
                prevC[nr[i]][nc[i]] = c;
                queueR[qTail] = nr[i]; queueC[qTail] = nc[i]; qTail++;
                if (qTail >= MAX_PATH) break;
            }
        }
    }
    if (!found) return false;

    // Walk back from end to start
    int tmpR[MAX_PATH], tmpC[MAX_PATH];
    int len = 0;
    int r = er, c = ec;
    while (r != -1) {
        tmpR[len] = r; tmpC[len] = c; len++;
        int pr = prevR[r][c], pc = prevC[r][c];
        r = pr; c = pc;
    }
    // reverse into truck path
    tr->pathLen = len;
    for (int i = 0; i < len; i++) {
        tr->pathR[i] = tmpR[len-1-i];
        tr->pathC[i] = tmpC[len-1-i];
    }
    return true;
}

static void SpawnTruck(void) {
    if (buildingCount < 2) return;

    for (int i = 0; i < MAX_TRUCKS; i++) {
        if (trucks[i].active) continue;

        int from = GetRandomValue(0, buildingCount-1);
        int to = GetRandomValue(0, buildingCount-1);
        int guard = 0;
        while (to == from && guard < 10) { to = GetRandomValue(0, buildingCount-1); guard++; }
        if (to == from) return;

        int sr, sc, er, ec;
        if (!NearestRoad(buildings[from].r, buildings[from].c, &sr, &sc)) return;
        if (!NearestRoad(buildings[to].r, buildings[to].c, &er, &ec)) return;

        Truck t = {0};
        if (!BFSPath(sr, sc, er, ec, &t)) return;
        t.idx = 0;
        t.t = 0;
        t.speed = 0.02f + GetRandomValue(0, 15)/1000.0f;
        t.color = (GetRandomValue(0,1) == 0) ? (Color){255,107,53,255} : (Color){242,193,78,255};
        t.active = true;
        trucks[i] = t;
        return;
    }
}

static void UpdateTrucks(void) {
    for (int i = 0; i < MAX_TRUCKS; i++) {
        if (!trucks[i].active) continue;
        Truck *tr = &trucks[i];
        if (tr->idx >= tr->pathLen - 1) {
            tr->active = false;
            deliveries++;
            continue;
        }
        tr->t += tr->speed;
        if (tr->t >= 1.0f) {
            tr->t = 0;
            tr->idx++;
            if (tr->idx >= tr->pathLen - 1) {
                tr->active = false;
                deliveries++;
            }
        }
    }
    // Keep a handful of trucks running
    int activeCount = 0;
    for (int i = 0; i < MAX_TRUCKS; i++) if (trucks[i].active) activeCount++;
    if (activeCount < 6 && GetRandomValue(0, 20) == 0) SpawnTruck();
}

static void DrawTrucks(void) {
    for (int i = 0; i < MAX_TRUCKS; i++) {
        if (!trucks[i].active) continue;
        Truck *tr = &trucks[i];
        Vector2 a = CellCenter(tr->pathR[tr->idx], tr->pathC[tr->idx]);
        Vector2 b = CellCenter(tr->pathR[tr->idx+1], tr->pathC[tr->idx+1]);
        float x = a.x + (b.x - a.x) * tr->t;
        float y = a.y + (b.y - a.y) * tr->t;
        DrawCircle((int)x, (int)y, TILE*0.16f, tr->color);
    }
}

static void AddBuilding(int r, int c, TileType type) {
    if (buildingCount >= MAX_BUILDINGS) return;
    buildings[buildingCount].r = r;
    buildings[buildingCount].c = c;
    buildings[buildingCount].type = type;
    buildingCount++;
}

static void RemoveBuildingAt(int r, int c) {
    for (int i = 0; i < buildingCount; i++) {
        if (buildings[i].r == r && buildings[i].c == c) {
            buildings[i] = buildings[buildingCount-1];
            buildingCount--;
            return;
        }
    }
}

int main(void) {
    InitWindow(SCREEN_W, SCREEN_H, "PyCity - alpha");
    SetTargetFPS(60);

    memset(grid, TILE_EMPTY, sizeof(grid));
    memset(trucks, 0, sizeof(trucks));

    Tool tool = TOOL_ROAD;
    bool paused = false;

    while (!WindowShouldClose()) {
        // ---- input ----
        if (IsKeyPressed(KEY_ONE))   tool = TOOL_ROAD;
        if (IsKeyPressed(KEY_TWO))   tool = TOOL_HOUSE;
        if (IsKeyPressed(KEY_THREE)) tool = TOOL_FACTORY;
        if (IsKeyPressed(KEY_FOUR))  tool = TOOL_BULLDOZE;
        if (IsKeyPressed(KEY_SPACE)) paused = !paused;

        Vector2 mouse = GetMousePosition();
        int c = (int)(mouse.x / TILE);
        int r = (int)((mouse.y - TOP_BAR) / TILE);
        bool hovering = InBounds(r,c) && mouse.y >= TOP_BAR;

        if (hovering && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            switch (tool) {
                case TOOL_ROAD:
                    if (grid[r][c] == TILE_EMPTY) grid[r][c] = TILE_ROAD;
                    break;
                case TOOL_HOUSE:
                    if (grid[r][c] == TILE_EMPTY) { grid[r][c] = TILE_HOUSE; AddBuilding(r,c,TILE_HOUSE); }
                    break;
                case TOOL_FACTORY:
                    if (grid[r][c] == TILE_EMPTY) { grid[r][c] = TILE_FACTORY; AddBuilding(r,c,TILE_FACTORY); }
                    break;
                case TOOL_BULLDOZE:
                    if (grid[r][c] != TILE_EMPTY) {
                        if (grid[r][c] == TILE_HOUSE || grid[r][c] == TILE_FACTORY) RemoveBuildingAt(r,c);
                        grid[r][c] = TILE_EMPTY;
                    }
                    break;
            }
        }

        // ---- update ----
        if (!paused) UpdateTrucks();

        // ---- draw ----
        BeginDrawing();
        ClearBackground((Color){21,26,24,255});

        // toolbar
        DrawRectangle(0, 0, SCREEN_W, TOP_BAR, (Color){28,35,33,255});
        const char *toolNames[4] = {"ROAD (1)", "HOUSE (2)", "FACTORY (3)", "BULLDOZE (4)"};
        DrawText(TextFormat("Tool: %s", toolNames[tool]), 10, 8, 18, (Color){255,107,53,255});
        DrawText(TextFormat("Deliveries: %d   Buildings: %d   %s", deliveries, buildingCount, paused ? "[PAUSED]" : ""),
                  10, 32, 16, (Color){237,232,222,255});

        // grid
        for (int rr = 0; rr < ROWS; rr++) {
            for (int cc = 0; cc < COLS; cc++) {
                int x = cc*TILE, y = TOP_BAR + rr*TILE;
                Color base = (Color){30,38,35,255};
                switch (grid[rr][cc]) {
                    case TILE_ROAD:    base = (Color){58,67,64,255}; break;
                    case TILE_HOUSE:   base = (Color){76,110,156,255}; break;
                    case TILE_FACTORY: base = (Color){255,107,53,255}; break;
                    default: break;
                }
                DrawRectangle(x, y, TILE-1, TILE-1, base);
            }
        }

        // hover highlight
        if (hovering) {
            DrawRectangleLines(c*TILE, TOP_BAR + r*TILE, TILE, TILE, (Color){242,193,78,255});
        }

        DrawTrucks();

        EndDrawing();
    }

    CloseWindow();
    return 0;
}