#include "raylib.h"
#include "perlin_noise.h"
#include <vector>
#include <cmath>
#include <raymath.h>
#include <limits>

const int WORLD_SIZE = 64;
const int MAX_HEIGHT = 20;
const int OCTAVES = 2;
const float NOISE_SCALE = 0.05f;
const float BLOCK_SIZE = 1.0f;
const float GRAVITY = 9.8f;

bool generatedTerrain = false;
Texture grassTexture;
Texture dirtTexture;
Texture bedrockTexture;
struct Block {
  Vector3 position;
  Texture texture;
};

std::vector<Block> blocks;
std::vector<std::vector<std::vector<bool>>> solid(WORLD_SIZE,
  std::vector<std::vector<bool>>(MAX_HEIGHT, std::vector<bool>(WORLD_SIZE, false)));

bool IsExposed(int x, int y, int z, const std::vector<std::vector<std::vector<bool>>>& heightmap) {
  return (x == 0 || !heightmap[x - 1][y][z]) ||
    (x == WORLD_SIZE - 1 || !heightmap[x + 1][y][z]) ||
    (y == 0 || !heightmap[x][y - 1][z]) ||
    (y == MAX_HEIGHT - 1 || !heightmap[x][y + 1][z]) ||
    (z == 0 || !heightmap[x][y][z - 1]) ||
    (z == WORLD_SIZE - 1 || !heightmap[x][y][z + 1]);
}



void GenerateTerrain() {
  for (int x = 0; x < WORLD_SIZE; x++) {
    for (int z = 0; z < WORLD_SIZE; z++) {
      float noise = perlinNoise(x * NOISE_SCALE, z * NOISE_SCALE, OCTAVES);
      int height = (int)((noise + 1.0f) / 2.0f * MAX_HEIGHT);

      if (height >= MAX_HEIGHT) height = MAX_HEIGHT - 1;
      for (int y = 0; y <= height; y++) {
        solid[x][y][z] = true;
      }
    }
  }

  for (int x = 0; x < WORLD_SIZE; x++) {
    for (int z = 0; z < WORLD_SIZE; z++) {
      for (int y = 0; y < MAX_HEIGHT; y++) {
        if (!solid[x][y][z]) continue;
        if (!IsExposed(x, y, z, solid)) continue;

        Block block;
        block.position = { (float)x, (float)y, (float)z };

        if (y == MAX_HEIGHT - 1 || (y + 1 < MAX_HEIGHT && !solid[x][y + 1][z]))
          block.texture = grassTexture;
        else if (y > 1)
          block.texture = dirtTexture;
        else
          block.texture = bedrockTexture;

        blocks.push_back(block);
      }
    }
  }
  generatedTerrain = true;
}

void RebuildBlocks() {
  blocks.clear();

  std::vector<int> topBlockY(WORLD_SIZE * WORLD_SIZE, 0);

  for (int x = 0; x < WORLD_SIZE; x++) {
    for (int z = 0; z < WORLD_SIZE; z++) {
      for (int y = MAX_HEIGHT - 1; y >= 0; y--) {
        if (solid[x][y][z]) {
          topBlockY[x * WORLD_SIZE + z] = y;
          break;
        }
      }
    }
  }

  for (int x = 0; x < WORLD_SIZE; x++) {
    for (int z = 0; z < WORLD_SIZE; z++) {
      int surfaceY = topBlockY[x * WORLD_SIZE + z];

      for (int y = 0; y < MAX_HEIGHT; y++) {
        if (!solid[x][y][z]) continue;
        if (!IsExposed(x, y, z, solid)) continue;

        Block block;
        block.position = { (float)x, (float)y, (float)z };

        if (y == surfaceY) {
          block.texture = grassTexture;
        }
        else if (y > 1) {
          block.texture = dirtTexture;
        }
        else {
          block.texture = bedrockTexture;
        }

        blocks.push_back(block);
      }
    }
  }
}


int main() {
  InitWindow(800, 600, "minecraft");
  Vector2 center = { GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
  Model cubeModel = LoadModel("resources/cube.obj");
  Mesh cubeMesh = cubeModel.meshes[0];
  grassTexture = LoadTexture("resources/grassblock.png");
  dirtTexture = LoadTexture("resources/dirt.png");
  bedrockTexture = LoadTexture("resources/bedrock.png");
  DisableCursor();
  SetTargetFPS(240);
  
  Camera3D camera = { 0 };
  camera.position = { 40.0f, 30.0f, 40.0f };
  camera.target = { 0.0f, 0.0f, 0.0f };
  camera.up = { 0.0f, 1.0f, 0.0f };
  camera.fovy = 45.0f;
  camera.projection = CAMERA_PERSPECTIVE;
  BoundingBox player;
  float yaw = 0;
  float pitch = 0;

  if (!generatedTerrain) {
    GenerateTerrain();
  }
  bool grounded = false;
  float playerVelocity = 0.0f;

  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(SKYBLUE);
    DrawFPS(10, 30);
    BeginMode3D(camera);
    UpdateCamera(&camera, CAMERA_FIRST_PERSON);

    if (IsKeyDown(KEY_LEFT_CONTROL)) {
      camera.position.y -= 0.1f;
    }
    if (IsKeyDown(KEY_SPACE)) {
      camera.position.y += 0.1f;
    }

    for (const Block& block : blocks) {
      cubeModel.materials[0].maps[0].texture = block.texture;
      DrawModel(cubeModel, block.position, 1.0f, GRAY);
    }
    
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
      Ray ray = GetMouseRay(center, camera);
      DrawLine3D(ray.position, Vector3Add(ray.position, Vector3Scale(ray.direction, 100)), GREEN);
      float closestDist = std::numeric_limits<float>::max();
      int selected = -1;

      for (int i = 0; i < blocks.size(); i++) {
        Vector3 pos = blocks[i].position;

        BoundingBox box;
        box.min = { pos.x - 0.5f, pos.y - 0.5f, pos.z - 0.5f };
        box.max = { pos.x + 0.5f, pos.y + 0.5f, pos.z + 0.5f };
        RayCollision hit = GetRayCollisionBox(ray, box);
        if (hit.hit && hit.distance < closestDist) {
          closestDist = hit.distance;
          selected = i;
        }
      }
      if (selected >= 0) {
        Vector3 pos = blocks[selected].position;
        solid[pos.x][pos.y][pos.z] = false;
        RebuildBlocks();
      }
    }
    
    EndMode3D();
    DrawText("WASD + mouse to move", 10, 10, 20, BLACK);
    DrawText(TextFormat("Current velocity: %0.2f", playerVelocity), 10, 50, 20, BLACK);
    DrawText(TextFormat("Grounded: %d", (int)grounded), 10, 70, 20, BLACK);
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
