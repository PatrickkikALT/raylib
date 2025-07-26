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
  Vector2 center = { GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
  Model cubeModel = LoadModel("resources/cube.obj");
  Mesh cubeMesh = cubeModel.meshes[0];
  Texture grassTexture = LoadTexture("resources/grassblock.png");
  Texture dirtTexture = LoadTexture("resources/dirt.png");
  Texture bedrockTexture = LoadTexture("resources/bedrock.png");
  DisableCursor();
  SetTargetFPS(240);
  
  Camera3D camera = { 0 };
  camera.position = { 40.0f, 30.0f, 40.0f };
  camera.target = { 0.0f, 0.0f, 0.0f };
  camera.up = { 0.0f, 1.0f, 0.0f };
  camera.fovy = 45.0f;
  camera.projection = CAMERA_PERSPECTIVE;

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

    float deltaTime = GetFrameTime();
    
    playerVelocity -= GRAVITY * deltaTime;

    if (grounded && IsKeyPressed(KEY_SPACE)) {
      playerVelocity = -6.0f;
      grounded = false;
    }

    Vector3 nextPos = camera.position;
    nextPos.y -= playerVelocity * deltaTime;

    int nx = (int)nextPos.x;
    int ny = (int)(nextPos.y - 1.0f);
    int nz = (int)nextPos.z;

    if (nx >= 0 && nx < WORLD_SIZE && ny >= 0 && ny < MAX_HEIGHT && nz >= 0 && nz < WORLD_SIZE) {
      if (!solid[nx][ny][nz]) {
        camera.position.y = nextPos.y;
        grounded = false;
      }
      else {
        grounded = true;
        playerVelocity = 0;
        camera.position.y = ny + 1.0f;
      }
    }


    //ground check
    Ray ray = { camera.position, { 0.0f, 1.0f, 0.0f} };
    ray.direction = Vector3{ 0.0f, -1.0f, 0.0f };

    float closestDist = FLT_MAX;
    Vector3 groundPos = { 0 };

    //bad for performance
    //for (Block& block : blocks) {
    //  RayCollision hit = GetRayCollisionMesh(ray, cubeMesh, MatrixTranslate(block.position.x, block.position.y, block.position.z));
    //  if (hit.hit && hit.distance < closestDist) {
    //    closestDist = hit.distance;
    //    groundPos = hit.point;
    //  }
    //}
    Vector3 camPos = camera.position;
    int x = (int)camPos.x;
    int z = (int)camPos.z;

    for (int y = (int)camPos.y; y >= 0; y--) {
      if (x >= 0 && x < WORLD_SIZE && y < MAX_HEIGHT && z >= 0 && z < WORLD_SIZE) {
        if (solid[x][y][z]) {
          groundPos.y = y + 1.4f;
          break;
        }
      }
    }

    camera.position.y += playerVelocity * deltaTime;
    //apply gravity
    if (camera.position.y >= groundPos.y + 1.4f) {
      camera.position.y -= GRAVITY * deltaTime;
    }

    //mouse movement

    Vector2 delta = GetMouseDelta();
    yaw -= delta.x * 0.002f;
    pitch -= delta.y * 0.002f;
    if (pitch > 89.0f * DEG2RAD) pitch = 89.0f * DEG2RAD;
    if (pitch < -89.0f * DEG2RAD) pitch = -89.0f * DEG2RAD;

    //movement
    Vector3 forward = {
      cosf(pitch) * sinf(yaw),
      sinf(pitch),
      cosf(pitch) * cosf(yaw)
    };
    Vector3 right = {
      sinf(yaw - PI / 2.0f),
      0.0f,
      cosf(yaw - PI / 2.0f)
    };

    Vector3 up = Vector3CrossProduct(right, forward);

    Vector3 oldPosition = camera.position;

    if (IsKeyDown(KEY_W)) camera.position = Vector3Add(camera.position, Vector3Scale(forward, 10 * deltaTime));
    if (IsKeyDown(KEY_S)) camera.position = Vector3Subtract(camera.position, Vector3Scale(forward, 10 * deltaTime));
    if (IsKeyDown(KEY_A)) camera.position = Vector3Subtract(camera.position, Vector3Scale(right, 10 * deltaTime));
    if (IsKeyDown(KEY_D)) camera.position = Vector3Add(camera.position, Vector3Scale(right, 10 * deltaTime));



    Vector3 testPos = camera.position;
    int tx = (int)testPos.x;
    int ty = (int)(testPos.y - 1.0f);
    int tz = (int)testPos.z;
    if (tx >= 0 && tx < WORLD_SIZE && ty >= 0 && ty < MAX_HEIGHT && tz >= 0 && tz < WORLD_SIZE) {
      if (solid[tx][ty][tz]) {
        camera.position = oldPosition;
      }
    }
    camera.target = Vector3Add(camera.position, forward);
    camera.up = up;

    for (const Block& block : blocks) {
      if (ColorIsEqual(block.color, GREEN)) {
        cubeModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = grassTexture;
      }
      else if (ColorIsEqual(block.color, BROWN)) {
        cubeModel.materials[0].maps[0].texture = dirtTexture;
      }
      else {
        cubeModel.materials[0].maps[0].texture = bedrockTexture;
      }
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
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
