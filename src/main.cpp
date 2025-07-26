#include "raylib.h"
#include "perlin_noise.h"
#include <vector>
#include <cmath>
#include <raymath.h>

const int WORLD_SIZE = 64;
const int MAX_HEIGHT = 20;
const int OCTAVES = 2;
const float NOISE_SCALE = 0.05f;
const float BLOCK_SIZE = 1.0f;

bool generatedTerrain = false;

struct Block {
  Vector3 position;
  Color color;
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
          block.color = GREEN;
        else if (y > 1)
          block.color = BROWN;
        else
          block.color = DARKGRAY;

        blocks.push_back(block);
      }
    }
  }
  generatedTerrain = true;
}

void RebuildBlocks() {
  blocks.clear();

  for (int x = 0; x < WORLD_SIZE; x++) {
    for (int z = 0; z < WORLD_SIZE; z++) {
      for (int y = 0; y < MAX_HEIGHT; y++) {
        if (!solid[x][y][z]) continue;
        if (!IsExposed(x, y, z, solid)) continue;

        Block block;
        block.position = { (float)x, (float)y, (float)z };

        if (y == MAX_HEIGHT - 1 || (y + 1 < MAX_HEIGHT && !solid[x][y + 1][z]))
          block.color = GREEN;
        else if (y > 1)
          block.color = BROWN;
        else
          block.color = DARKGRAY;

        blocks.push_back(block);
      }
    }
  }
}

int main() {
  InitWindow(800, 600, "minecraft");

  Mesh cubeMesh = GenMeshCube(BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE);
  Texture grassTexture = LoadTexture("resources/grass.png");
  Texture dirtTexture = LoadTexture("resources/dirt.png");
  Model cubeModel = LoadModelFromMesh(cubeMesh);
  DisableCursor();
  SetTargetFPS(240);
  
  Camera3D camera = { 0 };
  camera.position = { 40.0f, 30.0f, 40.0f };
  camera.target = { 0.0f, 0.0f, 0.0f };
  camera.up = { 0.0f, 1.0f, 0.0f };
  camera.fovy = 45.0f;
  camera.projection = CAMERA_PERSPECTIVE;

  if (!generatedTerrain) {
    GenerateTerrain();
  }

  while (!WindowShouldClose()) {
    UpdateCamera(&camera, CAMERA_FREE);

    BeginDrawing();
    ClearBackground(SKYBLUE);
    DrawFPS(10, 30);
    BeginMode3D(camera);

    for (const Block& block : blocks) {
      if (ColorIsEqual(block.color, GREEN)) {
        cubeModel.materials[0].maps[0].texture = grassTexture;
      }
      else if (ColorIsEqual(block.color, BROWN)) {
        cubeModel.materials[0].maps[0].texture = dirtTexture;
      }
      else {
        cubeModel.materials[0].maps[0].texture = { 0 };
      }
      DrawModel(cubeModel, block.position, 1.0f, GRAY);
    }

    Ray ray = GetMouseRay(GetMousePosition(), camera);
    float closestDist = 1000.0f;
    Vector3 hitBlock = { 0 };


    EndMode3D();
    DrawText("WASD + mouse to move", 10, 10, 20, BLACK);
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
