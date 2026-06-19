#include "client/input.h"
#include "raylib.h"
#include "shared/board.h"
#include "shared/log.h"
#include <assert.h>
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "render.h"

#define CELL_WIDTH 64
#define FONT_SIZE 40

#define SCREEN_WIDTH ((2 * BOARD_SIZE + 3) * CELL_WIDTH)
#define SCREEN_HEIGHT ((BOARD_SIZE + 7) * CELL_WIDTH)
#define FRAME_RATE 120

typedef struct {
    Texture2D textures[NUM_SHIPS];
    Texture2D rotated_textures[NUM_SHIPS];
} GameAssets;

typedef struct {
    bool is_dragging[NUM_SHIPS];
    bool is_rotated[NUM_SHIPS];
    bool is_placed[NUM_SHIPS];
    bool is_confirmed;

    float invalid_board_timer;

    Position ship_coordinates[NUM_SHIPS];
    Rectangle ship_rectangles[NUM_SHIPS];
} UiState;

static GameAssets assets;
static RenderTexture2D render_target;
static UiState ui_state;

typedef struct {
    int x;
    int y;
} ScreenCoord;

extern InputData input_state;

// ==========================
// INITIALISATION AND CLEANUP
// ==========================

void reset_ui_ships(void) {
    ui_state.is_confirmed = false;
    ui_state.invalid_board_timer = 3.0f;

    int y = 750;
    for (int i = 0; i < NUM_SHIPS; i++) {
        ui_state.is_dragging[i] = false;
        ui_state.is_placed[i] = false;
        ui_state.is_rotated[i] = false;

        ui_state.ship_rectangles[i] = (Rectangle){
            .x = (i + 1) * 100,
            .y = y,
            .width = assets.textures[i].width,
            .height = assets.textures[i].height,
        };
    }
}

bool init_graphics(void) {
    SetTraceLogLevel(LOG_WARNING);

    // Ensure window can be resized
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_ALWAYS_RUN);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Battleship");

    SetWindowMinSize(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);

    // Virtual canvas to target when rendering
    render_target = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    SetTextureFilter(render_target.texture, TEXTURE_FILTER_BILINEAR);

    // Use logical colours for board
    GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, ColorToInt(WHITE));
    GuiSetStyle(BUTTON, BASE_COLOR_DISABLED, ColorToInt(LIGHTGRAY));

    // Make text larger
    GuiSetStyle(DEFAULT, TEXT_SIZE, 28);

    SetTargetFPS(FRAME_RATE);

    const char image_paths[NUM_SHIPS][50] = {
        "./assets/ShipBattleshipHull.png", "./assets/ShipCarrierHull.png",
        "./assets/ShipCruiserHull.png",    "./assets/ShipSubMarineHull.png",
        "./assets/ShipDestroyerHull.png",
    };

    for (int i = 0; i < NUM_SHIPS; i++) {
        Image image = LoadImage(image_paths[i]);
        assets.textures[i] = LoadTextureFromImage(image);
        ImageRotateCCW(&image);
        assets.rotated_textures[i] = LoadTextureFromImage(image);
        UnloadImage(image);

        if (!IsTextureValid(assets.textures[i]) ||
            !IsTextureValid(assets.rotated_textures[i])) {
            // Unload all the textures that have already been loaded.
            for (int j = 0; j < i; j++) {
                UnloadTexture(assets.textures[j]);
                UnloadTexture(assets.rotated_textures[j]);
            }

            // Also check if base loaded but rotated one failed
            if (IsTextureValid(assets.textures[i])) {
                UnloadTexture(assets.textures[i]);
            }
            LOG_ERROR("%s", "Failed to load texture!");
            return false;
        }
    }

    reset_ui_ships();
    ui_state.invalid_board_timer = 0.0f;

    return true;
}

bool is_window_open(void) { return !WindowShouldClose(); }

void cleanup_graphics(void) {
    for (int i = 0; i < NUM_SHIPS; i++) {
        UnloadTexture(assets.textures[i]);
        UnloadTexture(assets.rotated_textures[i]);
    }
    UnloadRenderTexture(render_target);
    CloseWindow();
}

// ============================
// COORDINATE AND LOGIC HELPERS
// ============================

static bool all_ships_placed(void) {
    for (int i = 0; i < NUM_SHIPS; i++) {
        if (!ui_state.is_placed[i]) {
            return false;
        }
    }
    return true;
}

static ScreenCoord cell_coordinates(Position coord, bool is_board_1) {
    int left_offset = is_board_1 ? CELL_WIDTH : (BOARD_SIZE + 2) * CELL_WIDTH;
    return (ScreenCoord){
        .x = left_offset + CELL_WIDTH * coord.x,
        .y = CELL_WIDTH + CELL_WIDTH * coord.y,
    };
}

static bool in_own_board(ScreenCoord coord) {
    return CELL_WIDTH <= coord.x && coord.x < CELL_WIDTH * (BOARD_SIZE + 1) &&
           CELL_WIDTH <= coord.y && coord.y < CELL_WIDTH * (BOARD_SIZE + 1);
}

static Position coordinates_to_cell(ScreenCoord coord) {
    assert(in_own_board(coord));
    return (Position){
        .x = (coord.x - CELL_WIDTH) / CELL_WIDTH,
        .y = (coord.y - CELL_WIDTH) / CELL_WIDTH,
    };
}

static Rectangle cell_bounds(Position coord, bool is_board_1) {
    ScreenCoord coords = cell_coordinates(coord, is_board_1);
    Rectangle bounds = {
        .x = (float)coords.x,
        .y = (float)coords.y,
        .width = (float)CELL_WIDTH,
        .height = (float)CELL_WIDTH,
    };
    return bounds;
}

// ===========
// INPUT LOGIC
// ===========

static void snap_to_grid(int ship_index) {
    Rectangle *rectangle = &ui_state.ship_rectangles[ship_index];

    ScreenCoord coords = {
        .x = rectangle->x + (CELL_WIDTH / 2),
        .y = rectangle->y + (CELL_WIDTH / 2),
    };

    if (!in_own_board(coords)) {
        // Rejected for head not being on board
        ui_state.is_placed[ship_index] = false;
        return;
    }

    Position c = coordinates_to_cell(coords);

    // Check for tail clipping
    int len = ship_length(ship_index);
    bool fits = ui_state.is_rotated[ship_index] ? (c.x + len <= BOARD_SIZE)
                                                : (c.y + len <= BOARD_SIZE);

    if (!fits) {
        // Rejected for hanging off board
        ui_state.is_placed[ship_index] = false;
    }

    ScreenCoord sc = cell_coordinates(c, true);
    ui_state.ship_coordinates[ship_index] = c;
    ui_state.ship_rectangles[ship_index].x = sc.x;
    ui_state.ship_rectangles[ship_index].y = sc.y;
    ui_state.is_placed[ship_index] = true;
}

static void update_ship_dragging(void) {
    if (ui_state.is_confirmed) {
        return;
    }

    // Process anything currently being dragged
    bool is_any_dragging = false;
    for (int i = 0; i < NUM_SHIPS; i++) {
        if (!ui_state.is_dragging[i]) {
            continue;
        }

        is_any_dragging = true;
        Rectangle *rectangle = &ui_state.ship_rectangles[i];

        // Move with mouse
        Vector2 mouse_delta = GetMouseDelta();
        rectangle->x += mouse_delta.x;
        rectangle->y += mouse_delta.y;

        // Handle rotation
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            ui_state.is_rotated[i] = !ui_state.is_rotated[i];

            // Rotate about cursor
            Vector2 mouse = GetMousePosition();
            float offset_x = mouse.x - rectangle->x;
            float offset_y = mouse.y - rectangle->y;

            float temp = rectangle->width;
            rectangle->width = rectangle->height;
            rectangle->height = temp;

            rectangle->x = mouse.x - offset_y;
            rectangle->y = mouse.y - offset_x;
        }

        // Handle dropping and snapping
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            ui_state.is_dragging[i] = false;
            // Look at centre of top-left cell
            snap_to_grid(i);
            LOG_DEBUG("%s", "Stopped dragging");
        }
        break; // stop looping, since at this stage we only check currently
               // dragging ship
    }

    // Now check for new interactions
    if (is_any_dragging) {
        return;
    }

    // Loop backwards to choose top most ship if any overlap
    for (int i = NUM_SHIPS - 1; i >= 0; i--) {
        Rectangle *rectangle = &ui_state.ship_rectangles[i];
        Vector2 mouse = GetMousePosition();

        if (!CheckCollisionPointRec(mouse, *rectangle)) {
            continue;
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            ui_state.is_dragging[i] = true;
            ui_state.is_placed[i] = false;
            break; // break to prevent moving two ships at the same time
        } else if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            // Rotate without dragging
            ui_state.is_rotated[i] = !ui_state.is_rotated[i];

            float offset_x = mouse.x - rectangle->x;
            float offset_y = mouse.y - rectangle->y;

            float temp = rectangle->width;
            rectangle->width = rectangle->height;
            rectangle->height = temp;

            rectangle->x = mouse.x - offset_y;
            rectangle->y = mouse.y - offset_x;

            snap_to_grid(i);

            break;
        }
    }
}

// ==============
// RENDER HELPERS
// ==============

static void draw_peg(Rectangle bounds, CellState cell) {
    float center_x = bounds.x + bounds.width / 2.0f;
    float center_y = bounds.y + bounds.height / 2.0f;
    float radius = bounds.width / 4.0f;

    if (cell == CELL_MISS) {
        DrawCircle(center_x, center_y, radius, BLUE);
        DrawCircleLines(center_x, center_y, radius, LIGHTGRAY);
    } else if (cell == CELL_HIT) {
        DrawCircle(center_x, center_y, radius, RED);
        DrawCircleLines(center_x, center_y, radius, DARKGRAY);
    } else if (cell == CELL_SUNK) {
        DrawCircle(center_x, center_y, radius, MAROON);
        DrawCircleLines(center_x, center_y, radius, BLACK);
        float offset = radius * 0.5f;
        DrawLineEx((Vector2){center_x - offset, center_y - offset},
                   (Vector2){center_x + offset, center_y + offset}, 3.0f,
                   WHITE);
        DrawLineEx((Vector2){center_x + offset, center_y - offset},
                   (Vector2){center_x - offset, center_y + offset}, 3.0f,
                   WHITE);
    }
}

static void draw_boards(const ClientState *state) {
    DrawText("Your board", CELL_WIDTH, 5, FONT_SIZE, DARKGRAY);
    DrawText("Opponent's board", CELL_WIDTH * 12, 5, FONT_SIZE, DARKGRAY);

    for (int x = 0; x < BOARD_SIZE; x++) {
        for (int y = 0; y < BOARD_SIZE; y++) {
            Rectangle bounds = cell_bounds((Position){x, y}, true);
            DrawRectangleRec(bounds, WHITE);
            DrawRectangleLinesEx(bounds, 2, GRAY);
        }
    }

    // Draw target grid
    // Lock buttons if it is not our turn
    if (state->current_state != UI_STATE_MY_TURN) {
        GuiSetState(STATE_DISABLED);
    }

    for (int x = 0; x < BOARD_SIZE; x++) {
        for (int y = 0; y < BOARD_SIZE; y++) {
            Rectangle bounds = cell_bounds((Position){.x = x, .y = y}, false);
            if (GuiButton(bounds, "")) {
                input_state = (InputData){
                    .grid_pos = {.x = x, .y = y},
                    .type = INPUT_FIRE,
                };
            }
        }
    }
    // unlock after we finish drawing the board
    if (state->current_state != UI_STATE_MY_TURN) {
        GuiSetState(STATE_NORMAL);
    }
}

static void draw_fleets_and_pegs(const ClientState *state) {
    // Draw known enemy ships
    for (int i = 0; i < NUM_SHIPS; i++) {
        if (!state->game.enemy_ships_sunk[i]) {
            continue;
        }

        PositionWithDirection pwd = state->game.enemy_ship_positions[i];

        // If the state says horizontal, use the base texture. Otherwise,
        // rotated!
        Texture2D texture =
            pwd.horizontal ? assets.rotated_textures[i] : assets.textures[i];

        ScreenCoord sc =
            cell_coordinates((Position){pwd.pos.x, pwd.pos.y}, false);
        DrawTexture(texture, sc.x, sc.y, WHITE);
    }

    // Draw own ships
    for (int i = 0; i < NUM_SHIPS; i++) {
        Texture2D texture = ui_state.is_rotated[i] ? assets.rotated_textures[i]
                                                   : assets.textures[i];
        int x = ui_state.ship_rectangles[i].x;
        int y = ui_state.ship_rectangles[i].y;
        DrawTexture(texture, x, y, WHITE);
        // FOR DEBUGGING
        // DrawRectangleLinesEx(ui_state.ship_rectangles[i], 1, RED);
    }

    // Draw hit/miss pegs on both boards
    for (int x = 0; x < BOARD_SIZE; x++) {
        for (int y = 0; y < BOARD_SIZE; y++) {
            // Check own board
            CellState my_cell = get_cell(state->game.my_board, x, y);
            if (my_cell == CELL_HIT || my_cell == CELL_SUNK ||
                my_cell == CELL_MISS) {
                Rectangle bounds = cell_bounds((Position){x, y}, true);
                draw_peg(bounds, my_cell);
            }

            // Check target board
            CellState target_cell = get_cell(state->game.target_board, x, y);
            if (target_cell == CELL_HIT || target_cell == CELL_SUNK ||
                target_cell == CELL_MISS) {
                Rectangle bounds = cell_bounds((Position){x, y}, false);
                draw_peg(bounds, target_cell);
            }
        }
    }
}

static void draw_confirm_button(void) {
    Rectangle confirm_rectangle = {
        .x = CELL_WIDTH * 9,
        .y = CELL_WIDTH * 12,
        .height = CELL_WIDTH,
        .width = CELL_WIDTH * 2,
    };
    if (!all_ships_placed()) {
        GuiSetState(STATE_DISABLED);
    }
    if (GuiButton(confirm_rectangle, "Confirm")) {
        ui_state.is_confirmed = true;
        LOG_INFO("%s", "Confirmed!");
        input_state = (InputData){
            .type = INPUT_PLACED_SHIPS,
        };
        for (int i = 0; i < NUM_SHIPS; i++) {
            input_state.ships[i] = (InitialShipState){
                .ship = i,
                .pwd.pos =
                    {
                        .x = ui_state.ship_coordinates[i].x,
                        .y = ui_state.ship_coordinates[i].y,
                    },
                .pwd.horizontal = ui_state.is_rotated[i],
            };
        }
    }
    if (!all_ships_placed()) {
        GuiSetState(STATE_NORMAL);
    }
}

static void draw_status_text(const ClientState *state) {
    const char *status_text = "";
    Color text_color = DARKGRAY;

    switch (state->current_state) {
    case UI_STATE_PLACING_SHIPS:
        status_text = ui_state.is_confirmed ? "WAITING FOR SERVER..."
                                            : "PLACE YOUR SHIPS";
        text_color = DARKBLUE;
        break;
    case UI_STATE_WAITING_FOR_OPPONENT:
        status_text = "WAITING FOR OPPONENT...";
        text_color = ORANGE;
        break;
    case UI_STATE_MY_TURN:
        status_text = "YOUR TURN: SELECT A TARGET!";
        text_color = DARKGREEN;
        break;
    case UI_STATE_OPPONENT_TURN:
        status_text = "OPPONENT'S TURN: BRACE FOR IMPACT!";
        text_color = MAROON;
        break;
    default:
        break;
    }

    if (status_text[0] != '\0') {
        int text_width = MeasureText(status_text, FONT_SIZE);
        int center_x = (SCREEN_WIDTH - text_width) / 2;

        // Push the text a little lower if we are currently showing the confirm
        // button
        int center_y = (state->current_state == UI_STATE_PLACING_SHIPS)
                           ? CELL_WIDTH * 14
                           : CELL_WIDTH * 12;

        DrawText(status_text, center_x, center_y, FONT_SIZE, text_color);
    }

    // Show error for bad ship placement
    if (ui_state.invalid_board_timer > 0.0f) {
        ui_state.invalid_board_timer -= GetFrameTime();

        const char *error_msg =
            "INVALID LAYOUT: Ships are overlapping or out of bounds!";
        int error_font_size = 20;
        int error_width = MeasureText(error_msg, error_font_size);

        DrawText(error_msg, (SCREEN_WIDTH - error_width) / 2,
                 CELL_WIDTH * 14 + 50, error_font_size, RED);
    }
}

static void draw_game_over(const ClientState *state) {
    // Game over overlay
    if (state->current_state != UI_STATE_GAME_OVER) {
        return;
    }
    // Background
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){30, 30, 30, 200});

    // Test if opponent disconnected
    bool all_enemy_sunk = true;
    for (int i = 0; i < NUM_SHIPS; i++) {
        if (!state->game.enemy_ships_sunk[i]) {
            all_enemy_sunk = false;
            break;
        }
    }
    bool opponent_fled = (state->game.i_won && !all_enemy_sunk);

    // Text
    const char *end_text;
    Color end_color;
    const char *sub_text;
    if (opponent_fled) {
        end_text = "FORFEIT";
        end_color = ORANGE;
        sub_text = "Your opponent disconnected.";
    } else if (state->game.i_won) {
        end_text = "VICTORY";
        end_color = GOLD;
        sub_text = "You sank the enemy fleet!";
    } else {
        end_text = "DEFEAT";
        end_color = RED;
        sub_text = "Your fleet was destroyed.";
    }

    const int title_font_size = 100;
    const int sub_font_size = 30;

    const int button_width = 200;
    const int button_height = 60;

    const int title_offset_y = -120;
    const int sub_offset_y = 10;
    const int button_offset_y = 80;

    const int center_x = SCREEN_WIDTH / 2;
    const int center_y = SCREEN_HEIGHT / 2;

    DrawText(end_text, center_x - (MeasureText(end_text, title_font_size) / 2),
             center_y + title_offset_y, title_font_size, end_color);
    DrawText(sub_text, center_x - (MeasureText(sub_text, sub_font_size) / 2),
             center_y + sub_offset_y, sub_font_size, LIGHTGRAY);

    // Quit button
    GuiSetState(STATE_NORMAL);
    Rectangle quit_bounds = {.x = center_x - (button_width / 2),
                             .y = center_y + button_offset_y,
                             .width = button_width,
                             .height = button_height};
    if (GuiButton(quit_bounds, "QUIT GAME")) {
        input_state = (InputData){.type = INPUT_QUIT};
        LOG_INFO("%s", "Player pressed Quit from game over screen.");
    }
}

// ================
// MAIN RENDER LOOP
// ================

void render_frame(const ClientState *state) {
    // Calculate mouse coords in case window has been stretched
    float scale_x = (float)GetScreenWidth() / SCREEN_WIDTH;
    float scale_y = (float)GetScreenHeight() / SCREEN_HEIGHT;
    float scale = (scale_x < scale_y) ? scale_x : scale_y; // Keep aspect ratio
    SetMouseOffset(-(GetScreenWidth() - (SCREEN_WIDTH * scale)) * 0.5f,
                   -(GetScreenHeight() - (SCREEN_HEIGHT * scale)) * 0.5f);
    SetMouseScale(1.0f / scale, 1.0f / scale);

    // Process ship placements first
    if (state->current_state == UI_STATE_PLACING_SHIPS) {
        update_ship_dragging();
    }

    // Draw onto virtual canvas
    BeginTextureMode(render_target);
    ClearBackground(RAYWHITE);

    draw_boards(state);
    draw_fleets_and_pegs(state);
    if (state->current_state == UI_STATE_PLACING_SHIPS) {
        draw_confirm_button();
    }
    draw_status_text(state);
    draw_game_over(state);

    EndTextureMode();

    // Now draw to actual window
    BeginDrawing();
    ClearBackground(BLACK); // Adds black bars if necessary

    Rectangle source = {0.0f, 0.0f, (float)render_target.texture.width,
                        -(float)render_target.texture.height};
    Rectangle dest = {
        (GetScreenWidth() - ((float)SCREEN_WIDTH * scale)) * 0.5f,
        (GetScreenHeight() - ((float)SCREEN_HEIGHT * scale)) * 0.5f,
        (float)SCREEN_WIDTH * scale, (float)SCREEN_HEIGHT * scale};

    DrawTexturePro(render_target.texture, source, dest, (Vector2){0, 0}, 0.0f,
                   WHITE);
    EndDrawing();
}
